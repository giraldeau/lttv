#ifndef _LTT_MARKER_DESC_H
#define _LTT_MARKER_DESC_H

/*
 * Marker description support header.
 *
 * Mathieu Desnoyers, August 2007
 * License: LGPL.
 */

#include <stdio.h>
#include <glib.h>
#include <ltt/marker-field.h>

//FIXME TEMP!
static inline GQuark ltt_enum_string_get(struct marker_field *f,
  gulong value)
{
  char tmp[1024] = "ENUM-";
  sprintf(&tmp[sizeof("ENUM-") - 1], "%lu", value);
  return g_quark_from_string(tmp);
}

#endif //_LTT_MARKER_DESC_H
