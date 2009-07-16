/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2007 Mathieu Desnoyers
 *
 * Complete rewrite from the original version made by XangXiu Yang.
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

#include <glib.h>

#include <ltt/event.h>
#include <ltt/ltt-types.h>
#include <ltt/ltt-private.h>
#include <ltt/marker.h>
#include <ltt/marker-field.h>

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


void ltt_event_position_set(LttEventPosition *ep, LttTracefile *tf,
        guint block, guint offset, guint64 tsc)
{
  ep->tracefile = tf;
  ep->block = block;
  ep->offset = offset;
  ep->tsc = tsc;
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
 * These functions extract data from an event after architecture specific
 * conversions
 ****************************************************************************/
guint32 ltt_event_get_unsigned(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;

  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    reverse_byte_order = LTT_GET_BO(e->tracefile);
  }

  switch(f->size) {
  case 1:
    {
      guint8 x = *(guint8 *)(e->data + f->offset);
      return (guint32) x;    
    }
    break;
  case 2:
    return (guint32)ltt_get_uint16(reverse_byte_order, e->data + f->offset);
    break;
  case 4:
    return (guint32)ltt_get_uint32(reverse_byte_order, e->data + f->offset);
    break;
  case 8:
  default:
    g_critical("ltt_event_get_unsigned : field size %li unknown", f->size);
    return 0;
    break;
  }
}

gint32 ltt_event_get_int(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;
  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    reverse_byte_order = LTT_GET_BO(e->tracefile);
  }

  switch(f->size) {
  case 1:
    {
      gint8 x = *(gint8 *)(e->data + f->offset);
      return (gint32) x;    
    }
    break;
  case 2:
    return (gint32)ltt_get_int16(reverse_byte_order, e->data + f->offset);
    break;
  case 4:
    return (gint32)ltt_get_int32(reverse_byte_order, e->data + f->offset);
    break;
  case 8:
  default:
    g_critical("ltt_event_get_int : field size %li unknown", f->size);
    return 0;
    break;
  }
}

guint64 ltt_event_get_long_unsigned(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;
  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    reverse_byte_order = LTT_GET_BO(e->tracefile);
  }
  
  switch(f->size) {
  case 1:
    {
      guint8 x = *(guint8 *)(e->data + f->offset);
      return (guint64) x;    
    }
    break;
  case 2:
    return (guint64)ltt_get_uint16(reverse_byte_order, e->data + f->offset);
    break;
  case 4:
    return (guint64)ltt_get_uint32(reverse_byte_order, e->data + f->offset);
    break;
  case 8:
    return ltt_get_uint64(reverse_byte_order, e->data + f->offset);
    break;
  default:
    g_critical("ltt_event_get_long_unsigned : field size %li unknown", f->size);
    return 0;
    break;
  }
}

gint64 ltt_event_get_long_int(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;
  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    reverse_byte_order = LTT_GET_BO(e->tracefile);
  }
  
  switch(f->size) {
  case 1:
    {
      gint8 x = *(gint8 *)(e->data + f->offset);
      return (gint64) x;    
    }
    break;
  case 2:
    return (gint64)ltt_get_int16(reverse_byte_order, e->data + f->offset);
    break;
  case 4:
    return (gint64)ltt_get_int32(reverse_byte_order, e->data + f->offset);
    break;
  case 8:
    return ltt_get_int64(reverse_byte_order, e->data + f->offset);
    break;
  default:
    g_critical("ltt_event_get_long_int : field size %li unknown", f->size);
    return 0;
    break;
  }
}

#if 0
float ltt_event_get_float(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;
  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    g_assert(LTT_HAS_FLOAT(e->tracefile));
    reverse_byte_order = LTT_GET_FLOAT_BO(e->tracefile);
  }

  g_assert(f->field_type.type_class == LTT_FLOAT && f->size == 4);

  if(reverse_byte_order == 0) return *(float *)(e->data + f->offset);
  else{
    void *ptr = e->data + f->offset;
    guint32 value = bswap_32(*(guint32*)ptr);
    return *(float*)&value;
  }
}

double ltt_event_get_double(LttEvent *e, struct marker_field *f)
{
  gboolean reverse_byte_order;
  if(unlikely(f->attributes & LTT_ATTRIBUTE_NETWORK_BYTE_ORDER)) {
    reverse_byte_order = (g_ntohs(0x1) != 0x1);
  } else {
    g_assert(LTT_HAS_FLOAT(e->tracefile));
    reverse_byte_order = LTT_GET_FLOAT_BO(e->tracefile);
  }

  if(f->size == 4)
    return ltt_event_get_float(e, f);
    
  g_assert(f->field_type.type_class == LTT_FLOAT && f->size == 8);

  if(reverse_byte_order == 0) return *(double *)(e->data + f->offset);
  else {
    void *ptr = e->data + f->offset;
    guint64 value = bswap_64(*(guint64*)ptr);
    return *(double*)&value;
  }
}
#endif

/*****************************************************************************
 * The string obtained is only valid until the next read from
 * the same tracefile. We reference directly the buffers.
 ****************************************************************************/
gchar *ltt_event_get_string(LttEvent *e, struct marker_field *f)
{
  g_assert(f->type == LTT_TYPE_STRING);

  //caused memory leaks
  //return (gchar*)g_strdup((gchar*)(e->data + f->offset));
  return (gchar*)(e->data + f->offset);
}


