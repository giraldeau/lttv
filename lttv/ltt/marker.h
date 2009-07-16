#ifndef _LTT_MARKERS_H
#define _LTT_MARKERS_H

/*
 * Marker support header.
 *
 * Mathieu Desnoyers, August 2007
 * License: LGPL.
 */

#include <glib.h>
#include <ltt/trace.h>
#include <ltt/compiler.h>
#include <ltt/marker-field.h>
#include <ltt/trace.h>

#define LTT_ATTRIBUTE_NETWORK_BYTE_ORDER (1<<1)

/* static ids 0-7 reserved for internal use. */
#define MARKER_CORE_IDS         8

struct marker_info;

struct marker_info {
  GQuark name;
  char *format;
  long size;         /* size if known statically, else -1 */
  guint8 largest_align; /* Size of the largest alignment needed in the
                           payload. */
  GArray *fields;    /* Array of struct marker_field */
  guint8 int_size, long_size, pointer_size, size_t_size;
  guint8 alignment;  /* Size on which the architecture alignment must be
                        done. Useful to encapsulate x86_32 events on
			x86_64 kernels. */
  struct marker_info *next; /* Linked list of markers with the same name */
};

struct marker_data {
  GArray *markers;			//indexed by marker id
  GHashTable *markers_hash;		//indexed by name hash
  GHashTable *markers_format_hash;	//indexed by name hash
};

enum marker_id {
  MARKER_ID_SET_MARKER_ID = 0,  /* Static IDs available (range 0-7) */
  MARKER_ID_SET_MARKER_FORMAT,
};

static inline guint16 marker_get_id_from_info(struct marker_data *data,
    struct marker_info *info)
{
  return ((unsigned long)info - (unsigned long)data->markers->data)
           / sizeof(struct marker_info);
}

static inline struct marker_info *marker_get_info_from_id(
    struct marker_data *data, guint16 id)
{
  if (unlikely(data->markers->len <= id))
    return NULL;
  return &g_array_index(data->markers, struct marker_info, id);
}

/*
 * Returns the head of the marker info list for that name.
 */
static inline struct marker_info *marker_get_info_from_name(
    struct marker_data *data, GQuark name)
{
  gpointer orig_key, value;
  int res;

  res = g_hash_table_lookup_extended(data->markers_hash,
    (gconstpointer)(gulong)name, &orig_key, &value);
  if (!res)
    return NULL;
  return marker_get_info_from_id(data, (guint16)(gulong)value);
}

static inline char *marker_get_format_from_name(struct marker_data *data,
    GQuark name)
{
  gpointer orig_key, value;
  int res;

  res = g_hash_table_lookup_extended(data->markers_format_hash,
 	 	(gconstpointer)(gulong)name, &orig_key, &value);
  if (!res)
    return NULL;
  return (char *)value;
}

static inline struct marker_field *marker_get_field(struct marker_info *info,
							guint i)
{
	return &g_array_index(info->fields, struct marker_field, i);
}

static inline unsigned int marker_get_num_fields(struct marker_info *info)
{
	return info->fields->len;
}

/*
 * for_each_marker_field  -  iterate over fields of a marker
 * @field:      struct marker_field * to use as iterator
 * @info:       marker info pointer
 */
#define for_each_marker_field(field, info)				\
	for (field = marker_get_field(info, 0);				\
		field != marker_get_field(info, marker_get_num_fields(info)); \
		field++)

int marker_format_event(LttTrace *trace, GQuark channel, GQuark name,
  const char *format);
int marker_id_event(LttTrace *trace, GQuark channel, GQuark name, guint16 id,
  uint8_t int_size, uint8_t long_size, uint8_t pointer_size,
  uint8_t size_t_size, uint8_t alignment);
struct marker_data *allocate_marker_data(void);
void destroy_marker_data(struct marker_data *data);

#endif //_LTT_MARKERS_H
