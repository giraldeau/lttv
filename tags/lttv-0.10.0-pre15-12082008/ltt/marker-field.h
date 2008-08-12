#ifndef _LTT_MARKERS_FIELD_H
#define _LTT_MARKERS_FIELD_H

/*
 * Marker field support header.
 *
 * Mathieu Desnoyers, August 2007
 * License: LGPL.
 */

#include <glib.h>

enum ltt_type {
	LTT_TYPE_SIGNED_INT,
	LTT_TYPE_UNSIGNED_INT,
	LTT_TYPE_POINTER,
	LTT_TYPE_STRING,
	LTT_TYPE_COMPACT,
	LTT_TYPE_NONE,
};

struct marker_field {
  GQuark name;
  enum ltt_type type;
  unsigned long offset; /* offset in the event data */
  unsigned long size;
  unsigned long alignment;
  unsigned long attributes;
  int static_offset;	/* boolean - private - is the field offset statically
  			 * known with the preceding types ? */
  GString *fmt;
};

static inline GQuark marker_field_get_name(struct marker_field *field)
{
	return field->name;
}

static inline enum ltt_type marker_field_get_type(struct marker_field *field)
{
	return field->type;
}

static inline unsigned long marker_field_get_size(struct marker_field *field)
{
	return field->size;
}

#endif //_LTT_MARKERS_FIELD_H
