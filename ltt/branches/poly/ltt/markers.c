/*
 * Marker support code.
 *
 * Mathieu Desnoyers, August 2007
 * License: LGPL.
 */

#include <glib.h>
#include <ltt/compiler.h>
#include <ltt/markers.h>

#define DEFAULT_MARKERS_NUM   100

int marker_format_event(LttTrace *trace, const char *name, const char *format)
{
  struct marker_info *info;
  
  info = g_hash_table_lookup(trace->markers_hash, (gconstpointer)name);
  if (!info)
    g_error("Got marker format %s, but marker name %s has no ID yet. "
            "Kernel issue.",
            format, name);
  if (info->format)
    g_free(info->format);
  info->format = g_new(char, strlen(format)+1);
  strcpy(info->format, format);
  /* TODO deal with format string */
}

int marker_id_event(LttTrace *trace, const char *name, uint16_t id)
{
  struct marker_info *info;

  if (trace->markers->len < id)
    trace->markers = g_array_set_size(trace->markers, id+1);
  info = &g_array_index(trace->markers, struct marker_info, id);
  if (info->name)
    g_free(info->name);
  info->name = g_new(char, strlen(name)+1);
  strcpy(info->name, name);
  g_hash_table_insert(trace->markers_hash, (gpointer)name, info);
}

int allocate_marker_data(LttTrace *trace)
{
  /* Init array to 0 */
  trace->markers = g_array_sized_new(FALSE, TRUE,
                    sizeof(struct marker_info), DEFAULT_MARKERS_NUM);
  trace->markers_hash = g_hash_table_new(g_str_hash, g_str_equal);
}

int destroy_marker_data(LttTrace *trace)
{
  unsigned int i;
  struct marker_info *info;

  for (i=0; i<trace->markers->len; i++) {
    info = &g_array_index(trace->markers, struct marker_info, i);
    if (info->name)
      g_free(info->name);
    if (info->format)
      g_free(info->format);
  }
  g_array_free(trace->markers, TRUE);
  g_hash_table_destroy(trace->markers_hash);
}
