/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2004-2005 Mathieu Desnoyers
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

#ifndef LTT_TYPES_H
#define LTT_TYPES_H

/* Set of functions to access the types portably, given the trace as parameter.
 * */

#include <ltt/ltt.h>
#include <ltt/ltt-private.h>
#include <glib.h>


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

inline gint64 ltt_get_int64(LttTrace t, void *ptr)
{
  return (gint64) (t->reverse_byte_order ? GUINT64_SWAP_LE_BE(x): x);
}


inline guint64 ltt_get_uint64(LttTrace t, void *ptr)
{
  return (guint64) (t->reverse_byte_order ? GUINT64_SWAP_LE_BE(x): x);
}

inline gint32 ltt_get_int32(LttTrace t, void *ptr)
{
  return (gint32) (t->reverse_byte_order ? GUINT32_SWAP_LE_BE(x): x);
}

inline guint32 ltt_get_uint32(LttTrace t, void *ptr)
{
  return (guint32) (t->reverse_byte_order ? GUINT32_SWAP_LE_BE(x): x);
}

inline gint16 ltt_get_int16(LttTrace t, void *ptr)
{
  return (gint16) (t->reverse_byte_order ? GUINT16_SWAP_LE_BE(x): x);
}

inline guint16 ltt_get_uint16(LttTrace t, void *ptr)
{
  return (guint16) (t->reverse_byte_order ? GUINT16_SWAP_LE_BE(x): x);
}

#endif // LTT_TYPES_H
