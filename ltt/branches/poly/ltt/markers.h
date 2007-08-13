/*
 * Marker support header.
 *
 * Mathieu Desnoyers, Auguest 2007
 * License: LGPL.
 */

#include <ltt/trace.h>
#include <ltt/compiler.h>

struct marker_info {
	char *name;
	char *format;
  unsigned long size;   /* size if known statically, else 0 */
};

enum marker_id {
  MARKER_ID_SET_MARKER_ID = 0,  /* Static IDs available (range 0-7) */
  MARKER_ID_SET_MARKER_FORMAT,
  MARKER_ID_HEARTBEAT_32,
  MARKER_ID_HEARTBEAT_64,
  MARKER_ID_COMPACT,    /* Compact IDs (range: 8-127)      */
  MARKER_ID_DYNAMIC,    /* Dynamic IDs (range: 128-65535)   */
};

static inline uint16_t marker_get_id_from_info(LttTrace *trace,
    struct marker_info *info)
{
  return ((unsigned long)info - (unsigned long)trace->markers->data)
           / sizeof(struct marker_info);
}

static inline struct marker_info *marker_get_info_from_id(LttTrace *trace,
    uint16_t id)
{
  if (unlikely(trace->markers->len < id))
    return NULL;
  return &g_array_index(trace->markers, struct marker_info, id);
}

int marker_format_event(LttTrace *trace, const char *name, const char *format);
int marker_id_event(LttTrace *trace, const char *name, uint16_t id);
int allocate_marker_data(LttTrace *trace);
int destroy_marker_data(LttTrace *trace);
