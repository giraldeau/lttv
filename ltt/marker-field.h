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

/*
 * Fields "offset" and "size" below are only valid while the event is being
 * read. They are also being shared with events of the same type coming from
 * other per-cpu tracefiles. Therefore, the "LttEvent" fields_offsets offset and
 * size should be used rather than these.
 */
struct marker_field {
  GQuark name;
  enum ltt_type type;
  unsigned int index;	/* Field index within the event */
  unsigned long _offset; /* offset in the event data, USED ONLY INTERNALLY BY LIB */
  unsigned long _size;	/* size of field. USED ONLY INTERNALLY BY LIB */
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

/*
 * Returns 0 if size is not known statically.
 */
static inline long marker_field_get_size(struct marker_field *field)
{
	return field->_size;
}

static inline unsigned int marker_field_get_index(struct marker_field *field)
{
	return field->index;
}

#endif //_LTT_MARKERS_FIELD_H
