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

unsigned ltt_event_eventtype_id(LttEvent *e)
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

LttFacility *ltt_event_facility(LttEvent *e)
{
  LttTrace * trace = e->tracefile->trace;
  unsigned id = e->event_id;
  return ltt_trace_facility_by_id(trace,id);
}

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype : get the event type of the event
 *Input params
 *    e                   : an instance of an event type   
 *Return value
 *    LttEventType *     : the event type of the event
 ****************************************************************************/

LttEventType *ltt_event_eventtype(LttEvent *e)
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

LttTime ltt_event_time(LttEvent *e)
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

LttCycleCount ltt_event_cycle_count(LttEvent *e)
{
  return e->tsc;
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
unsigned ltt_event_field_element_number(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return 0;
  
  if(f->field_type->type_class == LTT_ARRAY)
    return f->field_type->element_number;
  return (unsigned)  get_unsigned(LTT_GET_BO(e->tracefile),
      f->sequ_number_size, e + f->offset_root);
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_select
 *                   : Set the currently selected element for a sequence or
 *                     array field
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *    i              : the ith element
 ****************************************************************************/
void ltt_event_field_element_select(LttEvent *e, LttField *f, unsigned i)
{
  unsigned element_number;
  LttField *fld;
  unsigned int k;
  size_t size;
  void *data;
 
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return ;

  element_number  = ltt_event_field_element_number(e,f);
  /* Sanity check for i : 1..n only, and must be lower or equal element_number
   */
  if(element_number < i || i == 0) return;
  
  fld = f->child[0];
  
  data = e->data + f->offset_root;
  size = 0;
  for(k=0;k<i;k++){
    size += ltt_event_refresh_fields(f->offset_root+size,size, fld, evD+size,
                                LTT_GET_BO(e->tracefile));
  }
  f->current_element = i - 1;
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

  g_assert(t == LTT_UINT || t == LTT_ENUM);

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

  g_assert( f->field_type->type_class == LTT_INT);

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
