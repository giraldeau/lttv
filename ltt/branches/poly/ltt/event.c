/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
 *               2006 Mathieu Desnoyers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include <asm/types.h>
#include <byteswap.h>

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/event.h>
#include <ltt/trace.h>
#include <ltt/ltt-types.h>



void compute_fields_offsets(LttTracefile *tf,
    LttFacility *fac, LttField *field, off_t *offset, void *root);


LttEvent *ltt_event_new()
{
  return g_new(LttEvent, 1);
}

void ltt_event_destroy(LttEvent *event)
{
  g_free(event);
}


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
  if(f->field_type.type_class != LTT_ARRAY &&
     f->field_type.type_class != LTT_SEQUENCE)
    return 0;
  
  if(f->field_type.type_class == LTT_ARRAY)
    return f->field_type.size;
  return ltt_event_get_long_unsigned(e, &g_array_index(f->field_type.fields,
                                                 LttField, 0));
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_select
 *                   : Set the currently selected element for a sequence or
 *                     array field
 *                     O(1) because of offset array.
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *    i              : the ith element (0, ...)
 *returns : the child field, at the right index, updated.
 ****************************************************************************/
LttField *ltt_event_field_element_select(LttEvent *e, LttField *f, gulong i)
{
  gulong element_number;
  LttField *field;
  unsigned int k;
  size_t size;
  LttEventType *event_type;
  off_t new_offset;
 
  if(f->field_type.type_class != LTT_ARRAY &&
     f->field_type.type_class != LTT_SEQUENCE)
    return NULL;

  element_number  = ltt_event_field_element_number(e,f);
  event_type = ltt_event_eventtype(e);
  /* Sanity check for i : 0..n-1 only, and must be lower or equal element_number
   */
  if(i >= element_number) return NULL;
 
  if(f->field_type.type_class == LTT_ARRAY) {
   field = &g_array_index(f->field_type.fields, LttField, 0);
  } else {
   field = &g_array_index(f->field_type.fields, LttField, 1);
  }

  if(field->field_size != 0) {
    if(f->array_offset + (i * field->field_size) == field->offset_root)
      return field; /* fixed length child, already at the right offset */
    else
      new_offset = f->array_offset + (i * field->field_size);
  } else {
    /* Var. len. child */
    new_offset = g_array_index(f->dynamic_offsets, off_t, i);
  }
  compute_fields_offsets(e->tracefile, 
      ltt_event_facility(e), field, &new_offset, e->data);

  return field;
}


off_t ltt_event_field_offset(LttEvent *e, LttField *f)
{
  return f->offset_root;
}



/*****************************************************************************
 * These functions extract data from an event after architecture specific
 * conversions
 ****************************************************************************/
guint32 ltt_event_get_unsigned(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);
  
  switch(f->field_size) {
  case 1:
    {
      guint8 x = *(guint8 *)(e->data + f->offset_root);
      return (guint32) x;    
    }
    break;
  case 2:
    return (guint32)ltt_get_uint16(reverse_byte_order, e->data + f->offset_root);
    break;
  case 4:
    return (guint32)ltt_get_uint32(reverse_byte_order, e->data + f->offset_root);
    break;
  case 8:
  default:
    g_critical("ltt_event_get_unsigned : field size %i unknown", f->field_size);
    return 0;
    break;
  }
}

gint32 ltt_event_get_int(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);
  
  switch(f->field_size) {
  case 1:
    {
      gint8 x = *(gint8 *)(e->data + f->offset_root);
      return (gint32) x;    
    }
    break;
  case 2:
    return (gint32)ltt_get_int16(reverse_byte_order, e->data + f->offset_root);
    break;
  case 4:
    return (gint32)ltt_get_int32(reverse_byte_order, e->data + f->offset_root);
    break;
  case 8:
  default:
    g_critical("ltt_event_get_int : field size %i unknown", f->field_size);
    return 0;
    break;
  }
}

guint64 ltt_event_get_long_unsigned(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);
  
  switch(f->field_size) {
  case 1:
    {
      guint8 x = *(guint8 *)(e->data + f->offset_root);
      return (guint64) x;    
    }
    break;
  case 2:
    return (guint64)ltt_get_uint16(reverse_byte_order, e->data + f->offset_root);
    break;
  case 4:
    return (guint64)ltt_get_uint32(reverse_byte_order, e->data + f->offset_root);
    break;
  case 8:
    return ltt_get_uint64(reverse_byte_order, e->data + f->offset_root);
    break;
  default:
    g_critical("ltt_event_get_long_unsigned : field size %i unknown", f->field_size);
    return 0;
    break;
  }
}

gint64 ltt_event_get_long_int(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);
  
  switch(f->field_size) {
  case 1:
    {
      gint8 x = *(gint8 *)(e->data + f->offset_root);
      return (gint64) x;    
    }
    break;
  case 2:
    return (gint64)ltt_get_int16(reverse_byte_order, e->data + f->offset_root);
    break;
  case 4:
    return (gint64)ltt_get_int32(reverse_byte_order, e->data + f->offset_root);
    break;
  case 8:
    return ltt_get_int64(reverse_byte_order, e->data + f->offset_root);
    break;
  default:
    g_critical("ltt_event_get_long_int : field size %i unknown", f->field_size);
    return 0;
    break;
  }
}

float ltt_event_get_float(LttEvent *e, LttField *f)
{
  g_assert(LTT_HAS_FLOAT(e->tracefile));
  gboolean reverse_byte_order = LTT_GET_FLOAT_BO(e->tracefile);

  g_assert(f->field_type.type_class == LTT_FLOAT && f->field_size == 4);

  if(reverse_byte_order == 0) return *(float *)(e->data + f->offset_root);
  else{
    void *ptr = e->data + f->offset_root;
    guint32 value = bswap_32(*(guint32*)ptr);
    return *(float*)&value;
  }
}

double ltt_event_get_double(LttEvent *e, LttField *f)
{
  g_assert(LTT_HAS_FLOAT(e->tracefile));
  gboolean reverse_byte_order = LTT_GET_FLOAT_BO(e->tracefile);

  if(f->field_size == 4)
    return ltt_event_get_float(e, f);
    
  g_assert(f->field_type.type_class == LTT_FLOAT && f->field_size == 8);

  if(reverse_byte_order == 0) return *(double *)(e->data + f->offset_root);
  else {
    void *ptr = e->data + f->offset_root;
    guint64 value = bswap_64(*(guint64*)ptr);
    return *(double*)&value;
  }
}

/*****************************************************************************
 * The string obtained is only valid until the next read from
 * the same tracefile.
 ****************************************************************************/
char *ltt_event_get_string(LttEvent *e, LttField *f)
{
  g_assert(f->field_type.type_class == LTT_STRING);

  return (gchar*)g_strdup((gchar*)(e->data + f->offset_root));
}

/*****************************************************************************
 *Function name
 *    compute_fields_offsets : set the precomputable offset of the fields
 *Input params 
 *    fac : facility
 *    field : the field
 *    offset : pointer to the current offset, must be incremented
 ****************************************************************************/


void compute_fields_offsets(LttTracefile *tf, 
    LttFacility *fac, LttField *field, off_t *offset, void *root)
{
  LttType *type = &field->field_type;

  switch(type->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
    case LTT_FLOAT:
    case LTT_ENUM:
      if(field->fixed_root == FIELD_VARIABLE) {
        /* Align offset on type size */
        *offset += ltt_align(*offset, get_alignment(field),
                             fac->alignment);
        /* remember offset */
        field->offset_root = *offset;
        /* Increment offset */
        *offset += field->field_size;
      } else {
        //g_debug("type before offset : %llu %llu %u\n", *offset,
        //    field->offset_root,
        //    field->field_size);
        *offset = field->offset_root;
        *offset += field->field_size;
        //g_debug("type after offset : %llu\n", *offset);
      }
      break;
    case LTT_STRING:
      if(field->fixed_root == FIELD_VARIABLE) {
        field->offset_root = *offset;
      }
      *offset += strlen((gchar*)(root+*offset)) + 1;
      /* Realign the data */
      *offset += ltt_align(*offset, fac->pointer_size,
                           fac->alignment);
      break;
    case LTT_ARRAY:
      g_assert(type->fields->len == 1);
      {
        off_t local_offset;
        LttField *child = &g_array_index(type->fields, LttField, 0);
        if(field->fixed_root == FIELD_VARIABLE) {
          *offset += ltt_align(*offset, get_alignment(field),
                              fac->alignment);
          /* remember offset */
          field->offset_root = *offset;
          field->array_offset = *offset;
        }
     
        if(field->field_size != 0) {
          /* Increment offset */
          /* field_size is the array size in bytes */
          *offset = field->offset_root + field->field_size;
        } else {
          guint i;
          *offset = field->array_offset;
          field->dynamic_offsets = g_array_set_size(field->dynamic_offsets,
                                                    0);
          for(i=0; i<type->size; i++) {
            g_array_append_val(field->dynamic_offsets, *offset);
            compute_fields_offsets(tf, fac, child, offset, root);
          }
        }
  //      local_offset = field->array_offset;
  //      /* Set the offset at position 0 */
  //      compute_fields_offsets(tf, fac, child, &local_offset, root);
      }
      break;
    case LTT_SEQUENCE:
      g_assert(type->fields->len == 2);
      {
        off_t local_offset;
        LttField *child;
        guint i;
        guint num_elem;
        if(field->fixed_root == FIELD_VARIABLE) {
          *offset += ltt_align(*offset, get_alignment(field),
                              fac->alignment);
          /* remember offset */
          field->offset_root = *offset;

          child = &g_array_index(type->fields, LttField, 0);
          compute_fields_offsets(tf, fac, child, offset, root);
          child = &g_array_index(type->fields, LttField, 1);
          *offset += ltt_align(*offset, get_alignment(child),
                               fac->alignment);
          field->array_offset = *offset;

        } else {
          child = &g_array_index(type->fields, LttField, 1);
        }
        *offset = field->array_offset;
        field->dynamic_offsets = g_array_set_size(field->dynamic_offsets,
                                                  0);
        num_elem = ltt_event_field_element_number(&tf->event, field);
        for(i=0; i<num_elem; i++) {
          g_array_append_val(field->dynamic_offsets, *offset);
          compute_fields_offsets(tf, fac, child, offset, root);
        }
        g_assert(num_elem == field->dynamic_offsets->len);

        /* Realign the data */
        *offset += ltt_align(*offset, fac->pointer_size,
                             fac->alignment);
        
 //       local_offset = field->array_offset;
 //       /* Set the offset at position 0 */
 //       compute_fields_offsets(tf, fac, child, &local_offset, root);
      }
      break;
    case LTT_STRUCT:
      { 
        LttField *child;
        guint i;
        gint ret=0;
        if(field->fixed_root == FIELD_VARIABLE) {
          *offset += ltt_align(*offset, get_alignment(fac, field),
                               fac->alignment);
          /* remember offset */
          field->offset_root = *offset;
        } else {
          *offset = field->offset_root;
        }
        for(i=0; i<type->fields->len; i++) {
          child = &g_array_index(type->fields, LttField, i);
          compute_fields_offsets(tf, fac, child, offset, root);
        }
      }
      break;
    case LTT_UNION:
      { 
        LttField *child;
        guint i;
        gint ret=0;
        if(field->fixed_root == FIELD_VARIABLE) {
          *offset += ltt_align(*offset, get_alignment(field),
                               fac->alignment);
          /* remember offset */
          field->offset_root = *offset;
        }
        for(i=0; i<type->fields->len; i++) {
          *offset = field->offset_root;
          child = &g_array_index(type->fields, LttField, i);
          compute_fields_offsets(tf, fac, child, offset, root);
        }
        *offset = field->offset_root + field->field_size;
      }
      break;
    case LTT_NONE:
    default:
      g_error("compute_fields_offsets : unknown type");
  }

}


/*****************************************************************************
 *Function name
 *    compute_offsets : set the dynamically computable offsets of an event type
 *Input params 
 *    tf : tracefile
 *    event : event type
 *
 ****************************************************************************/
void compute_offsets(LttTracefile *tf, LttFacility *fac,
    LttEventType *event, off_t *offset, void *root)
{
  guint i;

  /* compute all variable offsets */
  for(i=0; i<event->fields->len; i++) {
    //g_debug("computing offset %u of %u\n", i, event->fields->len-1);
    LttField *field = &g_array_index(event->fields, LttField, i);
    compute_fields_offsets(tf, fac, field, offset, root);
  }

}

