/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2004-2005 Mathieu Desnoyers
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

#ifndef LTT_TYPES_H
#define LTT_TYPES_H

/* Set of functions to access the types portably, given the trace as parameter.
 * */

#include <ltt/ltt.h>
#include <glib.h>
#include <ltt/time.h>


/*****************************************************************************
 *Function name
 *    ltt_get_int64        : get a 64 bits integer number
 *Input params 
 *    ptr                  : pointer to the integer
 *Return value
 *    gint64               : a 64 bits integer
 *
 * Takes care of endianness
 *
 ****************************************************************************/

static inline gint64 ltt_get_int64(gboolean reverse_byte_order, void *ptr)
{
  guint64 value = *(guint64*)ptr;
  return (gint64) (reverse_byte_order ? GUINT64_SWAP_LE_BE(value): value);
}


static inline guint64 ltt_get_uint64(gboolean reverse_byte_order, void *ptr)
{
  guint64 value = *(guint64*)ptr;
  return (guint64) (reverse_byte_order ? GUINT64_SWAP_LE_BE(value): value);
}

static inline gint32 ltt_get_int32(gboolean reverse_byte_order, void *ptr)
{
  guint32 value = *(guint32*)ptr;
  return (gint32) (reverse_byte_order ? GUINT32_SWAP_LE_BE(value): value);
}

static inline guint32 ltt_get_uint32(gboolean reverse_byte_order, void *ptr)
{
  guint32 value = *(guint32*)ptr;
  return (guint32) (reverse_byte_order ? GUINT32_SWAP_LE_BE(value): value);
}

static inline gint16 ltt_get_int16(gboolean reverse_byte_order, void *ptr)
{
  guint16 value = *(guint16*)ptr;
  return (gint16) (reverse_byte_order ? GUINT16_SWAP_LE_BE(value): value);
}

static inline guint16 ltt_get_uint16(gboolean reverse_byte_order, void *ptr)
{
  guint16 value = *(guint16*)ptr;
  return (guint16) (reverse_byte_order ? GUINT16_SWAP_LE_BE(value): value);
}

static inline LttTime ltt_get_time(gboolean reverse_byte_order, void *ptr)
{
  LttTime output;

  output.tv_sec = ltt_get_uint32(reverse_byte_order, ptr);
  ptr += sizeof(guint32);
  output.tv_nsec = ltt_get_uint32(reverse_byte_order, ptr);

  return output;
}

#endif // LTT_TYPES_H
