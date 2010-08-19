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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ltt/compiler.h>
#include <ltt/marker.h>
#include <ltt/ltt-private.h>

#define DEFAULT_MARKERS_NUM   100
#define DEFAULT_FIELDS_NUM    1
#define MAX_NAME_LEN          1024

static inline const char *parse_trace_type(struct marker_info *info,
    const char *fmt,
    char *trace_size, enum ltt_type *trace_type,
    unsigned long *attributes)
{
  int qualifier;    /* 'h', 'l', or 'L' for integer fields */
        /* 'z' support added 23/7/1999 S.H.    */
        /* 'z' changed to 'Z' --davidm 1/25/99 */
        /* 't' added for ptrdiff_t */

  /* parse attributes. */
  repeat:
    switch (*fmt) {
      case 'n':
        *attributes |= LTT_ATTRIBUTE_NETWORK_BYTE_ORDER;
        ++fmt;
        goto repeat;
    }

  /* get the conversion qualifier */
  qualifier = -1;
  if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
      *fmt =='Z' || *fmt == 'z' || *fmt == 't' ||
      *fmt == 'S' || *fmt == '1' || *fmt == '2' ||
      *fmt == '4' || *fmt == '8') {
    qualifier = *fmt;
    ++fmt;
    if (qualifier == 'l' && *fmt == 'l') {
      qualifier = 'L';
      ++fmt;
    }
  }

  switch (*fmt) {
    case 'c':
      *trace_type = LTT_TYPE_UNSIGNED_INT;
      *trace_size = sizeof(char);
      goto parse_end;
    case 's':
      *trace_type = LTT_TYPE_STRING;
      goto parse_end;
    case 'p':
      *trace_type = LTT_TYPE_POINTER;
      *trace_size = info->pointer_size;
      goto parse_end;
    case 'd':
    case 'i':
      *trace_type = LTT_TYPE_SIGNED_INT;
      break;
    case 'o':
    case 'u':
    case 'x':
    case 'X':
      *trace_type = LTT_TYPE_UNSIGNED_INT;
      break;
    default:
      if (!*fmt)
        --fmt;
      goto parse_end;
  }
  switch (qualifier) {
  case 'L':
    *trace_size = sizeof(long long);
    break;
  case 'l':
    *trace_size = info->long_size;
    break;
  case 'Z':
  case 'z':
    *trace_size = info->size_t_size;
    break;
  case 't':
    *trace_size = info->pointer_size;
    break;
  case 'h':
    *trace_size = sizeof(short);
    break;
  case '1':
    *trace_size = sizeof(uint8_t);
    break;
  case '2':
    *trace_size = sizeof(guint16);
    break;
  case '4':
    *trace_size = sizeof(uint32_t);
    break;
  case '8':
    *trace_size = sizeof(uint64_t);
    break;
  default:
    *trace_size = info->int_size;
  }

parse_end:
  return fmt;
}

/*
 * Restrictions:
 * Field width and precision are *not* supported.
 * %n not supported.
 */
__attribute__((no_instrument_function))
static inline const char *parse_c_type(struct marker_info *info,
    const char *fmt,
    char *c_size, enum ltt_type *c_type, GString *field_fmt)
{
  int qualifier;    /* 'h', 'l', or 'L' for integer fields */
        /* 'z' support added 23/7/1999 S.H.    */
        /* 'z' changed to 'Z' --davidm 1/25/99 */
        /* 't' added for ptrdiff_t */

  /* process flags : ignore standard print formats for now. */
  repeat:
    switch (*fmt) {
      case '-':
      case '+':
      case ' ':
      case '#':
      case '0':
        g_string_append_c(field_fmt, *fmt);
        ++fmt;
        goto repeat;
    }

  /* get the conversion qualifier */
  qualifier = -1;
  if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
      *fmt =='Z' || *fmt == 'z' || *fmt == 't' ||
      *fmt == 'S') {
    qualifier = *fmt;
    ++fmt;
    if (qualifier == 'l' && *fmt == 'l') {
      qualifier = 'L';
      ++fmt;
    }
  }

  switch (*fmt) {
    case 'c':
      *c_type = LTT_TYPE_UNSIGNED_INT;
      *c_size = sizeof(unsigned char);
      g_string_append_c(field_fmt, *fmt);
      goto parse_end;
    case 's':
      *c_type = LTT_TYPE_STRING;
      goto parse_end;
    case 'p':
      *c_type = LTT_TYPE_POINTER;
      *c_size = info->pointer_size;
      goto parse_end;
    case 'd':
    case 'i':
      *c_type = LTT_TYPE_SIGNED_INT;
      g_string_append_c(field_fmt, 'l');
      g_string_append_c(field_fmt, 'l');
      g_string_append_c(field_fmt, *fmt);
      break;
    case 'o':
    case 'u':
    case 'x':
    case 'X':
      g_string_append_c(field_fmt, 'l');
      g_string_append_c(field_fmt, 'l');
      g_string_append_c(field_fmt, *fmt);
      *c_type = LTT_TYPE_UNSIGNED_INT;
      break;
    default:
      if (!*fmt)
        --fmt;
      goto parse_end;
  }
  switch (qualifier) {
  case 'L':
    *c_size = sizeof(long long);
    break;
  case 'l':
    *c_size = info->long_size;
    break;
  case 'Z':
  case 'z':
    *c_size = info->size_t_size;
    break;
  case 't':
    *c_size = info->pointer_size;
    break;
  case 'h':
    *c_size = sizeof(short);
    break;
  default:
    *c_size = info->int_size;
  }

parse_end:
  return fmt;
}

static inline long add_type(struct marker_info *info,
    long offset, const char *name,
    char trace_size, enum ltt_type trace_type,
    char c_size, enum ltt_type c_type, unsigned long attributes,
    unsigned int field_count, GString *field_fmt)
{
  struct marker_field *field;
  char tmpname[MAX_NAME_LEN];

  info->fields = g_array_set_size(info->fields, info->fields->len+1);
  field = &g_array_index(info->fields, struct marker_field,
            info->fields->len-1);
  if (name)
    field->name = g_quark_from_string(name);
  else {
    snprintf(tmpname, MAX_NAME_LEN-1, "field %u", field_count);
    field->name = g_quark_from_string(tmpname);
  }
  field->type = trace_type;
  field->fmt = g_string_new(field_fmt->str);

  switch (trace_type) {
  case LTT_TYPE_SIGNED_INT:
  case LTT_TYPE_UNSIGNED_INT:
  case LTT_TYPE_POINTER:
    field->size = trace_size;
    field->alignment = trace_size;
    info->largest_align = max((guint8)field->alignment,
                              (guint8)info->largest_align);
    field->attributes = attributes;
    if (offset == -1) {
      field->offset = -1;
      field->static_offset = 0;
      return -1;
    } else {
      field->offset = offset + ltt_align(offset, field->alignment,
                                         info->alignment);
      field->static_offset = 1;
      return field->offset + trace_size;
    }
  case LTT_TYPE_STRING:
    field->offset = offset;
    field->size = 0;  /* Variable length, size is 0 */
    field->alignment = 1;
    if (offset == -1)
      field->static_offset = 0;
    else
      field->static_offset = 1;
    return -1;
  default:
    g_error("Unexpected type");
    return 0;
  }
}

long marker_update_fields_offsets(struct marker_info *info, const char *data)
{
  struct marker_field *field;
  unsigned int i;
  long offset = 0;

  /* Find the last field with a static offset, then update from there. */
  for (i = info->fields->len - 1; i >= 0; i--) {
    field = &g_array_index(info->fields, struct marker_field, i);
    if (field->static_offset) {
      offset = field->offset;
      break;
    }
  }

  for (; i < info->fields->len; i++) {
    field = &g_array_index(info->fields, struct marker_field, i);

    switch (field->type) {
    case LTT_TYPE_SIGNED_INT:
    case LTT_TYPE_UNSIGNED_INT:
    case LTT_TYPE_POINTER:
      field->offset = offset + ltt_align(offset, field->alignment,
                                          info->alignment);
      offset = field->offset + field->size;
      break;
    case LTT_TYPE_STRING:
      field->offset = offset;
      offset = offset + strlen(&data[offset]) + 1;
      // not aligning on pointer size, breaking genevent backward compatibility.
      break;
    default:
      g_error("Unexpected type");
      return -1;
    }
  }
  return offset;
}

static void format_parse(const char *fmt, struct marker_info *info)
{
  char trace_size = 0, c_size = 0;  /*
             * 0 (unset), 1, 2, 4, 8 bytes.
             */
  enum ltt_type trace_type = LTT_TYPE_NONE, c_type = LTT_TYPE_NONE;
  unsigned long attributes = 0;
  long offset = 0;
  const char *name_begin = NULL, *name_end = NULL;
  char *name = NULL;
  unsigned int field_count = 1;
  GString *field_fmt = g_string_new("");

  name_begin = fmt;
  for (; *fmt ; ++fmt) {
    switch (*fmt) {
    case '#':
      /* tracetypes (#) */
      ++fmt;      /* skip first '#' */
      if (*fmt == '#') {  /* Escaped ## */
        g_string_append_c(field_fmt, *fmt);
        g_string_append_c(field_fmt, *fmt);
        break;
      }
      attributes = 0;
      fmt = parse_trace_type(info, fmt, &trace_size, &trace_type,
        &attributes);
      break;
    case '%':
      /* c types (%) */
      g_string_append_c(field_fmt, *fmt);
      ++fmt;      /* skip first '%' */
      if (*fmt == '%') {  /* Escaped %% */
        g_string_append_c(field_fmt, *fmt);
        break;
      }
      fmt = parse_c_type(info, fmt, &c_size, &c_type, field_fmt);
      /*
       * Output c types if no trace types has been
       * specified.
       */
      if (!trace_size)
        trace_size = c_size;
      if (trace_type == LTT_TYPE_NONE)
        trace_type = c_type;
      if (c_type == LTT_TYPE_STRING)
        trace_type = LTT_TYPE_STRING;
      /* perform trace write */
      offset = add_type(info, offset, name, trace_size,
            trace_type, c_size, c_type, attributes, field_count++,
	    field_fmt);
      trace_size = c_size = 0;
      trace_type = c_size = LTT_TYPE_NONE;
      g_string_truncate(field_fmt, 0);
      attributes = 0;
      name_begin = NULL;
      if (name) {
        g_free(name);
        name = NULL;
      }
      break;
    case ' ':
      g_string_truncate(field_fmt, 0);
      if (!name_end && name_begin) {
        name_end = fmt;
        if (name)
          g_free(name);
        name = g_new(char, name_end - name_begin + 1);
        memcpy(name, name_begin, name_end - name_begin);
        name[name_end - name_begin] = '\0';
      }
      break;  /* Skip white spaces */
    default:
      g_string_append_c(field_fmt, *fmt);
      if (!name_begin) {
        name_begin = fmt;
        name_end = NULL;
      }
    }
  }
  info->size = offset;
  if (name)
    g_free(name);
  g_string_free(field_fmt, TRUE);
}

int marker_parse_format(const char *format, struct marker_info *info)
{
  if (info->fields)
    g_array_free(info->fields, TRUE);
  info->fields = g_array_sized_new(FALSE, TRUE,
                    sizeof(struct marker_field), DEFAULT_FIELDS_NUM);
  format_parse(format, info);
  return 0;
}

int marker_format_event(LttTrace *trace, GQuark channel, GQuark name,
  const char *format)
{
  struct marker_info *info;
  struct marker_data *mdata;
  char *fquery;
  char *fcopy;
  GArray *group;

  group = g_datalist_id_get_data(&trace->tracefiles, channel);
  if (!group)
    return -ENOENT;
  g_assert(group->len > 0);
  mdata = g_array_index (group, LttTracefile, 0).mdata;

  fquery = marker_get_format_from_name(mdata, name);
  if (fquery) {
    if (strcmp(fquery, format) != 0)
      g_error("Marker format mismatch \"%s\" vs \"%s\" for marker %s.%s. "
            "Kernel issue.", fquery, format,
            g_quark_to_string(channel), g_quark_to_string(name));
    else
      return 0;  /* Already exists. Nothing to do. */
  }
  fcopy = g_new(char, strlen(format)+1);
  strcpy(fcopy, format);
  g_hash_table_insert(mdata->markers_format_hash, (gpointer)(gulong)name,
    (gpointer)fcopy);

  info = marker_get_info_from_name(mdata, name);
  for (; info != NULL; info = info->next) {
    info->format = fcopy;
    if (marker_parse_format(format, info))
      g_error("Error parsing marker format \"%s\" for marker \"%.s.%s\"",
        format, g_quark_to_string(channel), g_quark_to_string(name));
  }
  return 0;
}

int marker_id_event(LttTrace *trace, GQuark channel, GQuark name, guint16 id,
  uint8_t int_size, uint8_t long_size, uint8_t pointer_size,
  uint8_t size_t_size, uint8_t alignment)
{
  struct marker_data *mdata;
  struct marker_info *info, *head;
  int found = 0;
  GArray *group;

  g_debug("Add channel %s event %s %hu\n", g_quark_to_string(channel),
    g_quark_to_string(name), id);

  group = g_datalist_id_get_data(&trace->tracefiles, channel);
  if (!group)
    return -ENOENT;
  g_assert(group->len > 0);
  mdata = g_array_index (group, LttTracefile, 0).mdata;

  if (mdata->markers->len <= id)
    mdata->markers = g_array_set_size(mdata->markers,
      max(mdata->markers->len * 2, id + 1));
  info = &g_array_index(mdata->markers, struct marker_info, id);
  info->name = name;
  info->int_size = int_size;
  info->long_size = long_size;
  info->pointer_size = pointer_size;
  info->size_t_size = size_t_size;
  info->alignment = alignment;
  info->fields = NULL;
  info->next = NULL;
  info->format = marker_get_format_from_name(mdata, name);
  info->largest_align = 1;
  if (info->format && marker_parse_format(info->format, info))
      g_error("Error parsing marker format \"%s\" for marker \"%s.%s\"",
        info->format, g_quark_to_string(channel), g_quark_to_string(name));
  head = marker_get_info_from_name(mdata, name);
  if (!head)
    g_hash_table_insert(mdata->markers_hash, (gpointer)(gulong)name,
      (gpointer)(gulong)id);
  else {
    struct marker_info *iter;
    for (iter = head; iter != NULL; iter = iter->next)
      if (iter->name == name)
        found = 1;
    if (!found) {
      g_hash_table_replace(mdata->markers_hash, (gpointer)(gulong)name,
        (gpointer)(gulong)id);
      info->next = head;
    }
  }
  return 0;
}

struct marker_data *allocate_marker_data(void)
{
  struct marker_data *data;

  data = g_new(struct marker_data, 1);
  /* Init array to 0 */
  data->markers = g_array_sized_new(FALSE, TRUE,
                    sizeof(struct marker_info), DEFAULT_MARKERS_NUM);
  if (!data->markers)
    goto free_data;
  data->markers_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  if (!data->markers_hash)
    goto free_markers;
  data->markers_format_hash = g_hash_table_new_full(g_direct_hash,
     g_direct_equal, NULL, g_free);
  if (!data->markers_format_hash)
    goto free_markers_hash;
  return data;

  /* error handling */
free_markers_hash:
  g_hash_table_destroy(data->markers_hash);
free_markers:
  g_array_free(data->markers, TRUE);
free_data:
  g_free(data);
  return NULL;
}

void destroy_marker_data(struct marker_data *data)
{
  unsigned int i, j;
  struct marker_info *info;
  struct marker_field *field;

  for (i=0; i<data->markers->len; i++) {
    info = &g_array_index(data->markers, struct marker_info, i);
    if (info->fields) {
      for (j = 0; j < info->fields->len; j++) {
        field = &g_array_index(info->fields, struct marker_field, j);
	g_string_free(field->fmt, TRUE);
      }
      g_array_free(info->fields, TRUE);
    }
  }
  g_hash_table_destroy(data->markers_format_hash);
  g_hash_table_destroy(data->markers_hash);
  g_array_free(data->markers, TRUE);
  g_free(data);
}
