/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Mathieu Desnoyers
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

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <malloc.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

// For realpath
#include <limits.h>
#include <stdlib.h>


#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/trace.h>
#include <ltt/event.h>
#include <ltt/ltt-types.h>
#include <ltt/marker.h>

/* from marker.c */
extern long marker_update_fields_offsets(struct marker_info *info, const char *data);

/* Tracefile names used in this file */

GQuark LTT_TRACEFILE_NAME_METADATA;

#ifndef g_open
#define g_open open
#endif


#define __UNUSED__ __attribute__((__unused__))

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)

#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

#define g_close close

/* Those macros must be called from within a function where page_size is a known
 * variable */
#define PAGE_MASK (~(page_size-1))
#define PAGE_ALIGN(addr)  (((addr)+page_size-1)&PAGE_MASK)

LttTrace *father_trace = NULL;

/* set the offset of the fields belonging to the event,
   need the information of the archecture */
//void set_fields_offsets(LttTracefile *tf, LttEventType *event_type);
//size_t get_fields_offsets(LttTracefile *tf, LttEventType *event_type, void *data);

#if 0
/* get the size of the field type according to 
 * The facility size information. */
static inline void preset_field_type_size(LttTracefile *tf,
    LttEventType *event_type,
    off_t offset_root, off_t offset_parent,
    enum field_status *fixed_root, enum field_status *fixed_parent,
    LttField *field);
#endif //0

/* map a fixed size or a block information from the file (fd) */
static gint map_block(LttTracefile * tf, guint block_num);

/* calculate nsec per cycles for current block */
#if 0
static guint32 calc_nsecs_per_cycle(LttTracefile * t);
static guint64 cycles_2_ns(LttTracefile *tf, guint64 cycles);
#endif //0

/* go to the next event */
static int ltt_seek_next_event(LttTracefile *tf);

static int open_tracefiles(LttTrace *trace, gchar *root_path,
    gchar *relative_path);
static int ltt_process_metadata_tracefile(LttTracefile *tf);
static void ltt_tracefile_time_span_get(LttTracefile *tf,
                                        LttTime *start, LttTime *end);
static void group_time_span_get(GQuark name, gpointer data, gpointer user_data);
static gint map_block(LttTracefile * tf, guint block_num);
static void ltt_update_event_size(LttTracefile *tf);

/* Enable event debugging */
static int a_event_debug = 0;

void ltt_event_debug(int state)
{
  a_event_debug = state;
}

/* trace can be NULL
 *
 * Return value : 0 success, 1 bad tracefile
 */
static int parse_trace_header(ltt_subbuffer_header_t *header,
  LttTracefile *tf, LttTrace *t)
{
  if (header->magic_number == LTT_MAGIC_NUMBER)
    tf->reverse_bo = 0;
  else if(header->magic_number == LTT_REV_MAGIC_NUMBER)
    tf->reverse_bo = 1;
  else  /* invalid magic number, bad tracefile ! */
    return 1;
 
  if(t) {
    t->ltt_major_version = header->major_version;
    t->ltt_minor_version = header->minor_version;
    t->arch_size = header->arch_size;
  }
  tf->alignment = header->alignment;

  /* Get float byte order : might be different from int byte order
   * (or is set to 0 if the trace has no float (kernel trace)) */
  tf->float_word_order = 0;

  switch(header->major_version) {
  case 0:
  case 1:
    g_warning("Unsupported trace version : %hhu.%hhu",
          header->major_version, header->minor_version);
    return 1;
    break;
  case 2:
    switch(header->minor_version) {
    case 3:
      {
        struct ltt_subbuffer_header_2_3 *vheader = header;
        tf->buffer_header_size = ltt_subbuffer_header_size();
        tf->tscbits = 27;
        tf->eventbits = 5;
        tf->tsc_mask = ((1ULL << tf->tscbits) - 1);
        tf->tsc_mask_next_bit = (1ULL << tf->tscbits);

        if(t) {
          t->start_freq = ltt_get_uint64(LTT_GET_BO(tf),
                                         &vheader->start_freq);
          t->freq_scale = ltt_get_uint32(LTT_GET_BO(tf),
                                         &vheader->freq_scale);
          if(father_trace) {
            t->start_freq = father_trace->start_freq;
            t->freq_scale = father_trace->freq_scale;
          } else {
            father_trace = t;
          }
          t->start_tsc = ltt_get_uint64(LTT_GET_BO(tf),
                                        &vheader->cycle_count_begin);
          t->start_monotonic = 0;
          t->start_time.tv_sec = ltt_get_uint64(LTT_GET_BO(tf),
                                       &vheader->start_time_sec);
          t->start_time.tv_nsec = ltt_get_uint64(LTT_GET_BO(tf),
                                       &vheader->start_time_usec);
          t->start_time.tv_nsec *= 1000; /* microsec to nanosec */

          t->start_time_from_tsc = ltt_time_from_uint64(
              (double)t->start_tsc
              * 1000000000.0 * tf->trace->freq_scale
              / (double)t->start_freq);
        }
      }
      break;
    default:
      g_warning("Unsupported trace version : %hhu.%hhu",
              header->major_version, header->minor_version);
      return 1;
    }
    break;
  default:
    g_warning("Unsupported trace version : %hhu.%hhu",
            header->major_version, header->minor_version);
    return 1;
  }
  return 0;
}



/*****************************************************************************
 *Function name
 *    ltt_tracefile_open : open a trace file, construct a LttTracefile
 *Input params
 *    t                  : the trace containing the tracefile
 *    fileName           : path name of the trace file
 *    tf                 : the tracefile structure
 *Return value
 *                       : 0 for success, -1 otherwise.
 ****************************************************************************/ 

static gint ltt_tracefile_open(LttTrace *t, gchar * fileName, LttTracefile *tf)
{
  struct stat    lTDFStat;    /* Trace data file status */
  ltt_subbuffer_header_t *header;
  int page_size = getpagesize();

  //open the file
  tf->long_name = g_quark_from_string(fileName);
  tf->trace = t;
  tf->fd = open(fileName, O_RDONLY);
  if(tf->fd < 0){
    g_warning("Unable to open input data file %s\n", fileName);
    goto end;
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_warning("Unable to get the status of the input data file %s\n", fileName);
    goto close_file;
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size <
      (off_t)(ltt_subbuffer_header_size())){
    g_print("The input data file %s does not contain a trace\n", fileName);
    goto close_file;
  }
  
  /* Temporarily map the buffer start header to get trace information */
  /* Multiple of pages aligned head */
  tf->buffer.head = mmap(0,
      PAGE_ALIGN(ltt_subbuffer_header_size()), PROT_READ, 
      MAP_PRIVATE, tf->fd, 0);
  if(tf->buffer.head == MAP_FAILED) {
    perror("Error in allocating memory for buffer of tracefile");
    goto close_file;
  }
  g_assert( ( (gulong)tf->buffer.head&(8-1) ) == 0); // make sure it's aligned.
  
  header = (ltt_subbuffer_header_t *)tf->buffer.head;
  
  if(parse_trace_header(header, tf, NULL)) {
    g_warning("parse_trace_header error");
    goto unmap_file;
  }
    
  //store the size of the file
  tf->file_size = lTDFStat.st_size;
  tf->buf_size = ltt_get_uint32(LTT_GET_BO(tf), &header->buf_size);
  tf->num_blocks = tf->file_size / tf->buf_size;
  tf->events_lost = 0;
  tf->subbuf_corrupt = 0;

  if(munmap(tf->buffer.head,
        PAGE_ALIGN(ltt_subbuffer_header_size()))) {
    g_warning("unmap size : %zu\n",
        PAGE_ALIGN(ltt_subbuffer_header_size()));
    perror("munmap error");
    g_assert(0);
  }
  tf->buffer.head = NULL;

  //read the first block
  if(map_block(tf,0)) {
    perror("Cannot map block for tracefile");
    goto close_file;
  }
  
  return 0;

  /* Error */
unmap_file:
  if(munmap(tf->buffer.head,
        PAGE_ALIGN(ltt_subbuffer_header_size()))) {
    g_warning("unmap size : %zu\n",
        PAGE_ALIGN(ltt_subbuffer_header_size()));
    perror("munmap error");
    g_assert(0);
  }
close_file:
  close(tf->fd);
end:
  return -1;
}


/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, 
 *Input params
 *    t                  : tracefile which will be closed
 ****************************************************************************/

static void ltt_tracefile_close(LttTracefile *t)
{
  int page_size = getpagesize();

  if(t->buffer.head != NULL)
    if(munmap(t->buffer.head, PAGE_ALIGN(t->buf_size))) {
    g_warning("unmap size : %u\n",
        PAGE_ALIGN(t->buf_size));
    perror("munmap error");
    g_assert(0);
  }

  close(t->fd);
}

/****************************************************************************
 * get_absolute_pathname
 *
 * return the unique pathname in the system
 * 
 * MD : Fixed this function so it uses realpath, dealing well with
 * forgotten cases (.. were not used correctly before).
 *
 ****************************************************************************/
void get_absolute_pathname(const gchar *pathname, gchar * abs_pathname)
{
  abs_pathname[0] = '\0';

  if (realpath(pathname, abs_pathname) != NULL)
    return;
  else
  {
    /* error, return the original path unmodified */
    strcpy(abs_pathname, pathname);
    return;
  }
  return;
}

/* Search for something like : .*_.*
 *
 * The left side is the name, the right side is the number.
 * Exclude leading /.
 * Exclude flight- prefix.
 */

static int get_tracefile_name_number(gchar *raw_name,
                              GQuark *name,
                              guint *num,
                              gulong *tid,
                              gulong *pgid,
                              guint64 *creation)
{
  guint raw_name_len = strlen(raw_name);
  gchar char_name[PATH_MAX];
  int i;
  int underscore_pos;
  long int cpu_num;
  gchar *endptr;
  gchar *tmpptr;

  /* skip leading / */
  for(i = 0; i < raw_name_len-1;i++) {
    if(raw_name[i] != '/')
      break;
  }
  raw_name = &raw_name[i];
  raw_name_len = strlen(raw_name);

  for(i=raw_name_len-1;i>=0;i--) {
    if(raw_name[i] == '_') break;
  }
  if(i==-1) { /* Either not found or name length is 0 */
    /* This is a userspace tracefile */
    strncpy(char_name, raw_name, raw_name_len);
    char_name[raw_name_len] = '\0';
    *name = g_quark_from_string(char_name);
    *num = 0;  /* unknown cpu */
    for(i=0;i<raw_name_len;i++) {
      if(raw_name[i] == '/') {
        break;
      }
    }
    i++;
    for(;i<raw_name_len;i++) {
      if(raw_name[i] == '/') {
        break;
      }
    }
    i++;
    for(;i<raw_name_len;i++) {
      if(raw_name[i] == '-') {
        break;
      }
    }
    if(i == raw_name_len) return -1;
    i++;
    tmpptr = &raw_name[i];
    for(;i<raw_name_len;i++) {
      if(raw_name[i] == '.') {
        raw_name[i] = ' ';
        break;
      }
    }
    *tid = strtoul(tmpptr, &endptr, 10);
    if(endptr == tmpptr)
      return -1; /* No digit */
    if(*tid == ULONG_MAX)
      return -1; /* underflow / overflow */
    i++;
    tmpptr = &raw_name[i];
    for(;i<raw_name_len;i++) {
      if(raw_name[i] == '.') {
        raw_name[i] = ' ';
        break;
      }
    }
    *pgid = strtoul(tmpptr, &endptr, 10);
    if(endptr == tmpptr)
      return -1; /* No digit */
    if(*pgid == ULONG_MAX)
      return -1; /* underflow / overflow */
    i++;
    tmpptr = &raw_name[i];
    *creation = strtoull(tmpptr, &endptr, 10);
    if(endptr == tmpptr)
      return -1; /* No digit */
    if(*creation == G_MAXUINT64)
      return -1; /* underflow / overflow */
  } else {
    underscore_pos = i;

    cpu_num = strtol(raw_name+underscore_pos+1, &endptr, 10);

    if(endptr == raw_name+underscore_pos+1)
      return -1; /* No digit */
    if(cpu_num == LONG_MIN || cpu_num == LONG_MAX)
      return -1; /* underflow / overflow */
    
    if (!strncmp(raw_name, "flight-", sizeof("flight-") - 1)) {
      raw_name += sizeof("flight-") - 1;
      underscore_pos -= sizeof("flight-") - 1;
    }
    strncpy(char_name, raw_name, underscore_pos);
    char_name[underscore_pos] = '\0';
    *name = g_quark_from_string(char_name);
    *num = cpu_num;
  }
  
  
  return 0;
}


GData **ltt_trace_get_tracefiles_groups(LttTrace *trace)
{
  return &trace->tracefiles;
}


void compute_tracefile_group(GQuark key_id,
                             GArray *group,
                             struct compute_tracefile_group_args *args)
{
  unsigned int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      args->func(tf, args->func_args);
  }
}


static void ltt_tracefile_group_destroy(gpointer data)
{
  GArray *group = (GArray *)data;
  unsigned int i;
  LttTracefile *tf;

  if (group->len > 0)
    destroy_marker_data(g_array_index (group, LttTracefile, 0).mdata);
  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      ltt_tracefile_close(tf);
  }
  g_array_free(group, TRUE);
}

static __attribute__ ((__unused__)) gboolean ltt_tracefile_group_has_cpu_online(gpointer data)
{
  GArray *group = (GArray *)data;
  unsigned int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      return 1;
  }
  return 0;
}


/* Open each tracefile under a specific directory. Put them in a
 * GData : permits to access them using their tracefile group pathname.
 * i.e. access control/modules tracefile group by index :
 * "control/module".
 * 
 * relative path is the path relative to the trace root
 * root path is the full path
 *
 * A tracefile group is simply an array where all the per cpu tracefiles sit.
 */

static int open_tracefiles(LttTrace *trace, gchar *root_path, gchar *relative_path)
{
  DIR *dir = opendir(root_path);
  struct dirent *entry;
  struct stat stat_buf;
  int ret, i;
  struct marker_data *mdata;
  
  gchar path[PATH_MAX];
  int path_len;
  gchar *path_ptr;

  int rel_path_len;
  gchar rel_path[PATH_MAX];
  gchar *rel_path_ptr;
  LttTracefile tmp_tf;

  if(dir == NULL) {
    perror(root_path);
    return ENOENT;
  }

  strncpy(path, root_path, PATH_MAX-1);
  path_len = strlen(path);
  path[path_len] = '/';
  path_len++;
  path_ptr = path + path_len;

  strncpy(rel_path, relative_path, PATH_MAX-1);
  rel_path_len = strlen(rel_path);
  rel_path[rel_path_len] = '/';
  rel_path_len++;
  rel_path_ptr = rel_path + rel_path_len;
  
  while((entry = readdir(dir)) != NULL) {

    if(entry->d_name[0] == '.') continue;
    
    strncpy(path_ptr, entry->d_name, PATH_MAX - path_len);
    strncpy(rel_path_ptr, entry->d_name, PATH_MAX - rel_path_len);
    
    ret = stat(path, &stat_buf);
    if(ret == -1) {
      perror(path);
      continue;
    }
    
    g_debug("Tracefile file or directory : %s\n", path);
    
  //  if(strcmp(rel_path, "/eventdefs") == 0) continue;
    
    if(S_ISDIR(stat_buf.st_mode)) {

      g_debug("Entering subdirectory...\n");
      ret = open_tracefiles(trace, path, rel_path);
      if(ret < 0) continue;
    } else if(S_ISREG(stat_buf.st_mode)) {
      GQuark name;
      guint num;
      gulong tid, pgid;
      guint64 creation;
      GArray *group;
      num = 0;
      tid = pgid = 0;
      creation = 0;
      if(get_tracefile_name_number(rel_path, &name, &num, &tid, &pgid, &creation))
        continue; /* invalid name */
      
      g_debug("Opening file.\n");
      if(ltt_tracefile_open(trace, path, &tmp_tf)) {
        g_info("Error opening tracefile %s", path);

        continue; /* error opening the tracefile : bad magic number ? */
      }

      g_debug("Tracefile name is %s and number is %u", 
          g_quark_to_string(name), num);

      mdata = NULL;
      tmp_tf.cpu_online = 1;
      tmp_tf.cpu_num = num;
      tmp_tf.name = name;
      tmp_tf.tid = tid;
      tmp_tf.pgid = pgid;
      tmp_tf.creation = creation;
      group = g_datalist_id_get_data(&trace->tracefiles, name);
      if(group == NULL) {
        /* Elements are automatically cleared when the array is allocated.
         * It makes the cpu_online variable set to 0 : cpu offline, by default.
         */
        group = g_array_sized_new (FALSE, TRUE, sizeof(LttTracefile), 10);
        g_datalist_id_set_data_full(&trace->tracefiles, name,
                                 group, ltt_tracefile_group_destroy);
        mdata = allocate_marker_data();
        if (!mdata)
          g_error("Error in allocating marker data");
      }

      /* Add the per cpu tracefile to the named group */
      unsigned int old_len = group->len;
      if(num+1 > old_len)
        group = g_array_set_size(group, num+1);

      g_assert(group->len > 0);
      if (!mdata)
        mdata = g_array_index (group, LttTracefile, 0).mdata;

      g_array_index (group, LttTracefile, num) = tmp_tf;
      g_array_index (group, LttTracefile, num).event.tracefile = 
        &g_array_index (group, LttTracefile, num);
      for (i = 0; i < group->len; i++)
        g_array_index (group, LttTracefile, i).mdata = mdata;
    }
  }
  
  closedir(dir);

  return 0;
}


/* Presumes the tracefile is already seeked at the beginning. It makes sense,
 * because it must be done just after the opening */
static int ltt_process_metadata_tracefile(LttTracefile *tf)
{
  int err;
  
  while(1) {
    err = ltt_tracefile_read_seek(tf);
    if(err == EPERM) goto seek_error;
    else if(err == ERANGE) break; /* End of tracefile */

    err = ltt_tracefile_read_update_event(tf);
    if(err) goto update_error;

    /* The rules are :
     * It contains only core events :
     *  0 : set_marker_id
     *  1 : set_marker_format
     */
    if(tf->event.event_id >= MARKER_CORE_IDS) {
      /* Should only contain core events */
      g_warning("Error in processing metadata file %s, "
          "should not contain event id %u.", g_quark_to_string(tf->name),
          tf->event.event_id);
      err = EPERM;
      goto event_id_error;
    } else {
      char *pos;
      const char *channel_name, *marker_name, *format;
      uint16_t id;
      guint8 int_size, long_size, pointer_size, size_t_size, alignment;

      switch((enum marker_id)tf->event.event_id) {
        case MARKER_ID_SET_MARKER_ID:
          channel_name = pos = tf->event.data;
	  pos += strlen(channel_name) + 1;
	  marker_name = pos;
          g_debug("Doing MARKER_ID_SET_MARKER_ID of marker %s.%s",
	    channel_name, marker_name);
          pos += strlen(marker_name) + 1;
          pos += ltt_align((size_t)pos, sizeof(guint16), tf->alignment);
          id = ltt_get_uint16(LTT_GET_BO(tf), pos);
          g_debug("In MARKER_ID_SET_MARKER_ID of marker %s.%s id %hu",
	  	channel_name, marker_name, id);
          pos += sizeof(guint16);
          int_size = *(guint8*)pos;
          pos += sizeof(guint8);
          long_size = *(guint8*)pos;
          pos += sizeof(guint8);
          pointer_size = *(guint8*)pos;
          pos += sizeof(guint8);
          size_t_size = *(guint8*)pos;
          pos += sizeof(guint8);
          alignment = *(guint8*)pos;
          pos += sizeof(guint8);
          marker_id_event(tf->trace,
                          g_quark_from_string(channel_name),
                          g_quark_from_string(marker_name),
                          id, int_size, long_size,
                          pointer_size, size_t_size, alignment);
          break;
        case MARKER_ID_SET_MARKER_FORMAT:
          channel_name = pos = tf->event.data;
          pos += strlen(channel_name) + 1;
          marker_name = pos;
          g_debug("Doing MARKER_ID_SET_MARKER_FORMAT of marker %s.%s",
                  channel_name, marker_name);
          pos += strlen(marker_name) + 1;
          format = pos;
          pos += strlen(format) + 1;
          marker_format_event(tf->trace,
                              g_quark_from_string(channel_name),
                              g_quark_from_string(marker_name),
                              format);
          /* get information from dictionary TODO */
          break;
        default:
          g_warning("Error in processing metadata file %s, "
              "unknown event id %hhu.",
              g_quark_to_string(tf->name),
              tf->event.event_id);
          err = EPERM;
          goto event_id_error;
      }
    }
  }
  return 0;

  /* Error handling */
event_id_error:
update_error:
seek_error:
  g_warning("An error occured in metadata tracefile parsing");
  return err;
}

/*
 * Open a trace and return its LttTrace handle.
 *
 * pathname must be the directory of the trace
 */

LttTrace *ltt_trace_open(const gchar *pathname)
{
  gchar abs_path[PATH_MAX];
  LttTrace  * t;
  LttTracefile *tf;
  GArray *group;
  unsigned int i;
  int ret;
  ltt_subbuffer_header_t *header;
  DIR *dir;
  struct dirent *entry;
  struct stat stat_buf;
  gchar path[PATH_MAX];
  
  t = g_new(LttTrace, 1);
  if(!t) goto alloc_error;

  get_absolute_pathname(pathname, abs_path);
  t->pathname = g_quark_from_string(abs_path);

  g_datalist_init(&t->tracefiles);

  /* Test to see if it looks like a trace */
  dir = opendir(abs_path);
  if(dir == NULL) {
    perror(abs_path);
    goto open_error;
  }
  while((entry = readdir(dir)) != NULL) {
    strcpy(path, abs_path);
    strcat(path, "/");
    strcat(path, entry->d_name);
    ret = stat(path, &stat_buf);
    if(ret == -1) {
      perror(path);
      continue;
    }
  }
  closedir(dir);
  
  /* Open all the tracefiles */
  if(open_tracefiles(t, abs_path, "")) {
    g_warning("Error opening tracefile %s", abs_path);
    goto find_error;
  }
  
  /* Parse each trace metadata_N files : get runtime fac. info */
  group = g_datalist_id_get_data(&t->tracefiles, LTT_TRACEFILE_NAME_METADATA);
  if(group == NULL) {
    g_warning("Trace %s has no metadata tracefile", abs_path);
    goto find_error;
  }

  /*
   * Get the trace information for the metadata_0 tracefile.
   * Getting a correct trace start_time and start_tsc is insured by the fact
   * that no subbuffers are supposed to be lost in the metadata channel.
   * Therefore, the first subbuffer contains the start_tsc timestamp in its
   * buffer header.
   */
  g_assert(group->len > 0);
  tf = &g_array_index (group, LttTracefile, 0);
  header = (ltt_subbuffer_header_t *)tf->buffer.head;
  ret = parse_trace_header(header, tf, t);
  g_assert(!ret);

  t->num_cpu = group->len;
  
  //ret = allocate_marker_data(t);
  //if (ret)
  //  g_error("Error in allocating marker data");

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if (tf->cpu_online)
      if(ltt_process_metadata_tracefile(tf))
        goto find_error;
      //  goto metadata_error;
  }

  return t;

  /* Error handling */
//metadata_error:
//  destroy_marker_data(t);
find_error:
  g_datalist_clear(&t->tracefiles);
open_error:
  g_free(t);
alloc_error:
  return NULL;

}

/* Open another, completely independant, instance of a trace.
 *
 * A read on this new instance will read the first event of the trace.
 * 
 * When we copy a trace, we want all the opening actions to happen again :
 * the trace will be reopened and totally independant from the original.
 * That's why we call ltt_trace_open.
 */
LttTrace *ltt_trace_copy(LttTrace *self)
{
  return ltt_trace_open(g_quark_to_string(self->pathname));
}

/*
 * Close a trace
 */

void ltt_trace_close(LttTrace *t)
{
  g_datalist_clear(&t->tracefiles);
  g_free(t);
}


/*****************************************************************************
 * Get the start time and end time of the trace 
 ****************************************************************************/

void ltt_tracefile_time_span_get(LttTracefile *tf,
                                        LttTime *start, LttTime *end)
{
  int err;

  err = map_block(tf, 0);
  if(unlikely(err)) {
    g_error("Can not map block");
    *start = ltt_time_infinite;
  } else
    *start = tf->buffer.begin.timestamp;

  err = map_block(tf, tf->num_blocks - 1);  /* Last block */
  if(unlikely(err)) {
    g_error("Can not map block");
    *end = ltt_time_zero;
  } else
    *end = tf->buffer.end.timestamp;
}

struct tracefile_time_span_get_args {
  LttTrace *t;
  LttTime *start;
  LttTime *end;
};

static void group_time_span_get(GQuark name, gpointer data, gpointer user_data)
{
  struct tracefile_time_span_get_args *args =
          (struct tracefile_time_span_get_args*)user_data;

  GArray *group = (GArray *)data;
  unsigned int i;
  LttTracefile *tf;
  LttTime tmp_start;
  LttTime tmp_end;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online) {
      ltt_tracefile_time_span_get(tf, &tmp_start, &tmp_end);
      if(ltt_time_compare(*args->start, tmp_start)>0) *args->start = tmp_start;
      if(ltt_time_compare(*args->end, tmp_end)<0) *args->end = tmp_end;
    }
  }
}

/* return the start and end time of a trace */

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end)
{
  LttTime min_start = ltt_time_infinite;
  LttTime max_end = ltt_time_zero;
  struct tracefile_time_span_get_args args = { t, &min_start, &max_end };

  g_datalist_foreach(&t->tracefiles, &group_time_span_get, &args);
  
  if(start != NULL) *start = min_start;
  if(end != NULL) *end = max_end;
  
}


/* Seek to the first event in a tracefile that has a time equal or greater than
 * the time passed in parameter.
 *
 * If the time parameter is outside the tracefile time span, seek to the first
 * event or if after, return ERANGE.
 *
 * If the time parameter is before the first event, we have to seek specially to
 * there.
 *
 * If the time is after the end of the trace, return ERANGE.
 *
 * Do a binary search to find the right block, then a sequential search in the
 * block to find the event. 
 *
 * In the special case where the time requested fits inside a block that has no
 * event corresponding to the requested time, the first event of the next block
 * will be seeked.
 *
 * IMPORTANT NOTE : // FIXME everywhere...
 *
 * You MUST NOT do a ltt_tracefile_read right after a ltt_tracefile_seek_time :
 * you will jump over an event if you do.
 *
 * Return value : 0 : no error, the tf->event can be used
 *                ERANGE : time if after the last event of the trace
 *                otherwise : this is an error.
 *
 * */

int ltt_tracefile_seek_time(LttTracefile *tf, LttTime time)
{
  int ret = 0;
  int err;
  unsigned int block_num, high, low;

  /* seek at the beginning of trace */
  err = map_block(tf, 0);  /* First block */
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

 /* If the time is lower or equal the beginning of the trace,
  * go to the first event. */
  if(ltt_time_compare(time, tf->buffer.begin.timestamp) <= 0) {
    ret = ltt_tracefile_read(tf);
    if(ret == ERANGE) goto range;
    else if (ret) goto fail;
    goto found; /* There is either no event in the trace or the event points
                   to the first event in the trace */
  }

  err = map_block(tf, tf->num_blocks - 1);  /* Last block */
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

 /* If the time is after the end of the trace, return ERANGE. */
  if(ltt_time_compare(time, tf->buffer.end.timestamp) > 0) {
    goto range;
  }

  /* Binary search the block */
  high = tf->num_blocks - 1;
  low = 0;
  
  while(1) {
    block_num = ((high-low) / 2) + low;

    err = map_block(tf, block_num);
    if(unlikely(err)) {
      g_error("Can not map block");
      goto fail;
    }
    if(high == low) {
      /* We cannot divide anymore : this is what would happen if the time
       * requested was exactly between two consecutive buffers'end and start 
       * timestamps. This is also what would happend if we didn't deal with out
       * of span cases prior in this function. */
      /* The event is right in the buffer!
       * (or in the next buffer first event) */
      while(1) {
        ret = ltt_tracefile_read(tf);
        if(ret == ERANGE) goto range; /* ERANGE or EPERM */
        else if(ret) goto fail;

        if(ltt_time_compare(time, tf->event.event_time) <= 0)
          goto found;
      }

    } else if(ltt_time_compare(time, tf->buffer.begin.timestamp) < 0) {
      /* go to lower part */
      high = block_num - 1;
    } else if(ltt_time_compare(time, tf->buffer.end.timestamp) > 0) {
      /* go to higher part */
      low = block_num + 1;
    } else {/* The event is right in the buffer!
               (or in the next buffer first event) */
      while(1) {
        ret = ltt_tracefile_read(tf);
        if(ret == ERANGE) goto range; /* ERANGE or EPERM */
        else if(ret) goto fail;

        if(ltt_time_compare(time, tf->event.event_time) <= 0)
          break;
      }
      goto found;
    }
  }

found:
  return 0;
range:
  return ERANGE;

  /* Error handling */
fail:
  g_error("ltt_tracefile_seek_time failed on tracefile %s", 
      g_quark_to_string(tf->name));
  return EPERM;
}

/* Seek to a position indicated by an LttEventPosition
 */

int ltt_tracefile_seek_position(LttTracefile *tf, const LttEventPosition *ep)
{
  int err;
  
  if(ep->tracefile != tf) {
    goto fail;
  }

  err = map_block(tf, ep->block);
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

  tf->event.offset = ep->offset;

  /* Put back the event real tsc */
  tf->event.tsc = ep->tsc;
  tf->buffer.tsc = ep->tsc;

  err = ltt_tracefile_read_update_event(tf);
  if(err) goto fail;

  /* deactivate this, as it does nothing for now
  err = ltt_tracefile_read_op(tf);
  if(err) goto fail;
  */

  return 0;

fail:
  g_error("ltt_tracefile_seek_time failed on tracefile %s", 
      g_quark_to_string(tf->name));
  return 1;
}

/* Given a TSC value, return the LttTime (seconds,nanoseconds) it
 * corresponds to.
 */

LttTime ltt_interpolate_time_from_tsc(LttTracefile *tf, guint64 tsc)
{
  LttTime time;
  
  if(tsc > tf->trace->start_tsc) {
    time = ltt_time_from_uint64(
        (double)(tsc - tf->trace->start_tsc) 
                                    * 1000000000.0 * tf->trace->freq_scale
                                    / (double)tf->trace->start_freq);
    time = ltt_time_add(tf->trace->start_time_from_tsc, time);
  } else {
    time = ltt_time_from_uint64(
        (double)(tf->trace->start_tsc - tsc)
                                    * 1000000000.0 * tf->trace->freq_scale
                                    / (double)tf->trace->start_freq);
    time = ltt_time_sub(tf->trace->start_time_from_tsc, time);
  }
  return time;
}

/* Calculate the real event time based on the buffer boundaries */
LttTime ltt_interpolate_time(LttTracefile *tf, LttEvent *event)
{
  return ltt_interpolate_time_from_tsc(tf, tf->buffer.tsc);
}


/* Get the current event of the tracefile : valid until the next read */
LttEvent *ltt_tracefile_get_event(LttTracefile *tf)
{
  return &tf->event;
}



/*****************************************************************************
 *Function name
 *    ltt_tracefile_read : Read the next event in the tracefile
 *Input params
 *    t                  : tracefile
 *Return value
 *
 *    Returns 0 if an event can be used in tf->event.
 *    Returns ERANGE on end of trace. The event in tf->event still can be used
 *    (if the last block was not empty).
 *    Returns EPERM on error.
 *
 *    This function does make the tracefile event structure point to the event
 *    currently pointed to by the tf->event.
 *
 *    Note : you must call a ltt_tracefile_seek to the beginning of the trace to
 *    reinitialize it after an error if you want results to be coherent.
 *    It would be the case if a end of trace last buffer has no event : the end
 *    of trace wouldn't be returned, but an error.
 *    We make the assumption there is at least one event per buffer.
 ****************************************************************************/

int ltt_tracefile_read(LttTracefile *tf)
{
  int err;

  err = ltt_tracefile_read_seek(tf);
  if(err) return err;
  err = ltt_tracefile_read_update_event(tf);
  if(err) return err;

  /* deactivate this, as it does nothing for now
  err = ltt_tracefile_read_op(tf);
  if(err) return err;
  */

  return 0;
}

int ltt_tracefile_read_seek(LttTracefile *tf)
{
  int err;

  /* Get next buffer until we finally have an event, or end of trace */
  while(1) {
    err = ltt_seek_next_event(tf);
    if(unlikely(err == ENOPROTOOPT)) {
      return EPERM;
    }

    /* Are we at the end of the buffer ? */
    if(err == ERANGE) {
      if(unlikely(tf->buffer.index == tf->num_blocks-1)){ /* end of trace ? */
        return ERANGE;
      } else {
        /* get next block */
        err = map_block(tf, tf->buffer.index + 1);
        if(unlikely(err)) {
          g_error("Can not map block");
          return EPERM;
        }
      }
    } else break; /* We found an event ! */
  }
  
  return 0;
}

/* do an operation when reading a new event */

/* This function does nothing for now */
#if 0
int ltt_tracefile_read_op(LttTracefile *tf)
{
  LttEvent *event;

  event = &tf->event;

  /* do event specific operation */

  /* nothing */

  return 0;
}
#endif

static void print_debug_event_header(LttEvent *ev, void *start_pos, void *end_pos)
{
  unsigned int offset = 0;
  int i, j;

  g_printf("Event header (tracefile %s offset %" PRIx64 "):\n",
    g_quark_to_string(ev->tracefile->long_name),
	   ((uint64_t)ev->tracefile->buffer.index * ev->tracefile->buf_size)
	   + (long)start_pos - (long)ev->tracefile->buffer.head);

  while (offset < (long)end_pos - (long)start_pos) {
    g_printf("%8lx", (long)start_pos - (long)ev->tracefile->buffer.head + offset);
    g_printf("    ");
    
    for (i = 0; i < 4 ; i++) {
      for (j = 0; j < 4; j++) {
	if (offset + ((i * 4) + j) <
		(long)end_pos - (long)start_pos)
          g_printf("%02hhX",
            ((char*)start_pos)[offset + ((i * 4) + j)]);
	else
	  g_printf("  ");
        g_printf(" ");
      }
      if (i < 4)
      g_printf(" ");
    }
    offset+=16;
    g_printf("\n");
  }
}


/* same as ltt_tracefile_read, but does not seek to the next event nor call
 * event specific operation. */
int ltt_tracefile_read_update_event(LttTracefile *tf)
{
  void * pos;
  LttEvent *event;
  void *pos_aligned;
 
  event = &tf->event;
  pos = tf->buffer.head + event->offset;

  /* Read event header */
  
  /* Align the head */
  pos += ltt_align((size_t)pos, sizeof(guint32), tf->alignment);
  pos_aligned = pos;
  
  event->timestamp = ltt_get_uint32(LTT_GET_BO(tf), pos);
  event->event_id = event->timestamp >> tf->tscbits;
  event->timestamp = event->timestamp & tf->tsc_mask;
  pos += sizeof(guint32);

  switch (event->event_id) {
  case 29:  /* LTT_RFLAG_ID_SIZE_TSC */
    event->event_id = ltt_get_uint16(LTT_GET_BO(tf), pos);
    pos += sizeof(guint16);
    event->event_size = ltt_get_uint16(LTT_GET_BO(tf), pos);
    pos += sizeof(guint16);
    if (event->event_size == 0xFFFF) {
      event->event_size = ltt_get_uint32(LTT_GET_BO(tf), pos);
      pos += sizeof(guint32);
    }
    pos += ltt_align((size_t)pos, sizeof(guint64), tf->alignment);
    tf->buffer.tsc = ltt_get_uint64(LTT_GET_BO(tf), pos);
    pos += sizeof(guint64);
    break;
  case 30:  /* LTT_RFLAG_ID_SIZE */
    event->event_id = ltt_get_uint16(LTT_GET_BO(tf), pos);
    pos += sizeof(guint16);
    event->event_size = ltt_get_uint16(LTT_GET_BO(tf), pos);
    pos += sizeof(guint16);
    if (event->event_size == 0xFFFF) {
      event->event_size = ltt_get_uint32(LTT_GET_BO(tf), pos);
      pos += sizeof(guint32);
    }
    break;
  case 31: /* LTT_RFLAG_ID */
    event->event_id = ltt_get_uint16(LTT_GET_BO(tf), pos);
    pos += sizeof(guint16);
    event->event_size = G_MAXUINT;
    break;
  default:
    event->event_size = G_MAXUINT;
    break;
  }

  if (likely(event->event_id != 29)) {
      /* No extended timestamp */
      if (event->timestamp < (tf->buffer.tsc & tf->tsc_mask))
        tf->buffer.tsc = ((tf->buffer.tsc & ~tf->tsc_mask)  /* overflow */
                            + tf->tsc_mask_next_bit)
                                | (guint64)event->timestamp;
      else
        tf->buffer.tsc = (tf->buffer.tsc & ~tf->tsc_mask)   /* no overflow */
                                | (guint64)event->timestamp;
  }
  event->tsc = tf->buffer.tsc;

  event->event_time = ltt_interpolate_time(tf, event);

  if (a_event_debug)
    print_debug_event_header(event, pos_aligned, pos);

  event->data = pos;

  /*
   * Let ltt_update_event_size update event->data according to the largest
   * alignment within the payload.
   * Get the data size and update the event fields with the current
   * information. */
  ltt_update_event_size(tf);

  return 0;
}


/****************************************************************************
 *Function name
 *    map_block       : map a block from the file
 *Input Params
 *    lttdes          : ltt trace file 
 *    whichBlock      : the block which will be read
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 ****************************************************************************/

static gint map_block(LttTracefile * tf, guint block_num)
{
  int page_size = getpagesize();
  ltt_subbuffer_header_t *header;

  g_assert(block_num < tf->num_blocks);

  if(tf->buffer.head != NULL) {
    if(munmap(tf->buffer.head, PAGE_ALIGN(tf->buf_size))) {
    g_warning("unmap size : %u\n",
        PAGE_ALIGN(tf->buf_size));
      perror("munmap error");
      g_assert(0);
    }
  }
  
  /* Multiple of pages aligned head */
  tf->buffer.head = mmap(0,
      PAGE_ALIGN(tf->buf_size),
      PROT_READ, MAP_PRIVATE, tf->fd,
      PAGE_ALIGN((off_t)tf->buf_size * (off_t)block_num));

  if(tf->buffer.head == MAP_FAILED) {
    perror("Error in allocating memory for buffer of tracefile");
    g_assert(0);
    goto map_error;
  }
  g_assert( ( (gulong)tf->buffer.head&(8-1) ) == 0); // make sure it's aligned.
  

  tf->buffer.index = block_num;

  header = (ltt_subbuffer_header_t *)tf->buffer.head;

  tf->buffer.begin.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              &header->cycle_count_begin);
  tf->buffer.begin.freq = tf->trace->start_freq;

  tf->buffer.begin.timestamp = ltt_interpolate_time_from_tsc(tf, 
                                          tf->buffer.begin.cycle_count);
  tf->buffer.end.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              &header->cycle_count_end);
  tf->buffer.end.freq = tf->trace->start_freq;
  
  tf->buffer.lost_size = ltt_get_uint32(LTT_GET_BO(tf),
                                        &header->lost_size);
  tf->buffer.end.timestamp = ltt_interpolate_time_from_tsc(tf,
                                        tf->buffer.end.cycle_count);
  tf->buffer.tsc =  tf->buffer.begin.cycle_count;
  tf->event.tsc = tf->buffer.tsc;
  tf->buffer.freq = tf->buffer.begin.freq;

  /* FIXME
   * eventually support variable buffer size : will need a partial pre-read of
   * the headers to create an index when we open the trace... eventually. */
  g_assert(tf->buf_size  == ltt_get_uint32(LTT_GET_BO(tf), 
                                             &header->buf_size));
  
  /* Make the current event point to the beginning of the buffer :
   * it means that the event read must get the first event. */
  tf->event.tracefile = tf;
  tf->event.block = block_num;
  tf->event.offset = 0;
  
  if (header->events_lost) {
    g_warning("%d events lost so far in tracefile %s at block %u",
      (guint)header->events_lost,
      g_quark_to_string(tf->long_name),
      block_num);
    tf->events_lost = header->events_lost;
  }
  if (header->subbuf_corrupt) {
    g_warning("%d subbuffer(s) corrupted so far in tracefile %s at block %u",
      (guint)header->subbuf_corrupt,
      g_quark_to_string(tf->long_name),
      block_num);
    tf->subbuf_corrupt = header->subbuf_corrupt;
  }

  return 0;

map_error:
  return -errno;
}

static void print_debug_event_data(LttEvent *ev)
{
  unsigned int offset = 0;
  int i, j;

  if (!max(ev->event_size, ev->data_size))
    return;

  g_printf("Event data (tracefile %s offset %" PRIx64 "):\n",
	   g_quark_to_string(ev->tracefile->long_name),
	   ((uint64_t)ev->tracefile->buffer.index * ev->tracefile->buf_size)
	   + (long)ev->data - (long)ev->tracefile->buffer.head);

  while (offset < max(ev->event_size, ev->data_size)) {
    g_printf("%8lx", (long)ev->data + offset
      - (long)ev->tracefile->buffer.head);
    g_printf("    ");
    
    for (i = 0; i < 4 ; i++) {
      for (j = 0; j < 4; j++) {
	if (offset + ((i * 4) + j) < max(ev->event_size, ev->data_size))
          g_printf("%02hhX", ((char*)ev->data)[offset + ((i * 4) + j)]);
	else
	  g_printf("  ");
        g_printf(" ");
      }
      if (i < 4)
      g_printf(" ");
    }

    g_printf("    ");

    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
	if (offset + ((i * 4) + j) < max(ev->event_size, ev->data_size)) {
	  if (isprint(((char*)ev->data)[offset + ((i * 4) + j)]))
            g_printf("%c", ((char*)ev->data)[offset + ((i * 4) + j)]);
	  else
            g_printf(".");
	} else
	  g_printf("  ");
      }
    }
    offset+=16;
    g_printf("\n");
  }
}

/* It will update the fields offsets too */
void ltt_update_event_size(LttTracefile *tf)
{
  off_t size = 0;
  struct marker_info *info;

  if (tf->name == LTT_TRACEFILE_NAME_METADATA) {
    switch((enum marker_id)tf->event.event_id) {
      case MARKER_ID_SET_MARKER_ID:
        size = strlen((char*)tf->event.data) + 1;
        g_debug("marker %s id set", (char*)tf->event.data + size);
        size += strlen((char*)tf->event.data + size) + 1;
        size += ltt_align(size, sizeof(guint16), tf->alignment);
        size += sizeof(guint16);
        size += sizeof(guint8);
        size += sizeof(guint8);
        size += sizeof(guint8);
        size += sizeof(guint8);
        size += sizeof(guint8);
        break;
      case MARKER_ID_SET_MARKER_FORMAT:
        size = strlen((char*)tf->event.data) + 1;
        g_debug("marker %s format set", (char*)tf->event.data);
        size += strlen((char*)tf->event.data + size) + 1;
        size += strlen((char*)tf->event.data + size) + 1;
        break;
    }
  }

  info = marker_get_info_from_id(tf->mdata, tf->event.event_id);

  if (tf->event.event_id >= MARKER_CORE_IDS)
    g_assert(info != NULL);

  /* Do not update field offsets of core markers when initially reading the
   * metadata tracefile when the infos about these markers do not exist yet.
   */
  if (likely(info && info->fields)) {
    /* alignment */
    tf->event.data += ltt_align((off_t)(unsigned long)tf->event.data,
    			     info->largest_align,
                             info->alignment);
    /* size, dynamically computed */
    if (info->size != -1)
      size = info->size;
    else
      size = marker_update_fields_offsets(marker_get_info_from_id(tf->mdata,
                                   tf->event.event_id), tf->event.data);
  }

  tf->event.data_size = size;
  
  /* Check consistency between kernel and LTTV structure sizes */
  if(tf->event.event_size == G_MAXUINT) {
    /* Event size too big to fit in the event size field */
    tf->event.event_size = tf->event.data_size;
  }

  if (a_event_debug)
    print_debug_event_data(&tf->event);

  if (tf->event.data_size != tf->event.event_size) {
    struct marker_info *info = marker_get_info_from_id(tf->mdata,
                                                       tf->event.event_id);
    if (!info)
      g_error("Undescribed event %hhu in channel %s", tf->event.event_id,
        g_quark_to_string(tf->name));
    g_error("Kernel/LTTV event size differs for event %s: kernel %u, LTTV %u",
        g_quark_to_string(info->name),
        tf->event.event_size, tf->event.data_size);
    exit(-1);
  }
}


/* Take the tf current event offset and use the event id to figure out where is
 * the next event offset.
 *
 * This is an internal function not aiming at being used elsewhere : it will
 * not jump over the current block limits. Please consider using
 * ltt_tracefile_read to do this.
 *
 * Returns 0 on success
 *         ERANGE if we are at the end of the buffer.
 *         ENOPROTOOPT if an error occured when getting the current event size.
 */
static int ltt_seek_next_event(LttTracefile *tf)
{
  int ret = 0;
  void *pos;
  
  /* seek over the buffer header if we are at the buffer start */
  if(tf->event.offset == 0) {
    tf->event.offset += tf->buffer_header_size;

    if(tf->event.offset == tf->buf_size - tf->buffer.lost_size) {
      ret = ERANGE;
    }
    goto found;
  }
 
  pos = tf->event.data;

  if(tf->event.data_size < 0) goto error;

  pos += (size_t)tf->event.data_size;
  
  tf->event.offset = pos - tf->buffer.head;
  
  if(tf->event.offset == tf->buf_size - tf->buffer.lost_size) {
    ret = ERANGE;
    goto found;
  }
  g_assert(tf->event.offset < tf->buf_size - tf->buffer.lost_size);

found:
  return ret;

error:
  g_error("Error in ltt_seek_next_event for tracefile %s",
      g_quark_to_string(tf->name));
  return ENOPROTOOPT;
}

#if 0
/*****************************************************************************
 *Function name
 *    set_fields_offsets : set the precomputable offset of the fields
 *Input params 
 *    tracefile       : opened trace file  
 *    event_type      : the event type
 ****************************************************************************/

void set_fields_offsets(LttTracefile *tf, LttEventType *event_type)
{
  LttField *field = event_type->root_field;
  enum field_status fixed_root = FIELD_FIXED, fixed_parent = FIELD_FIXED;

  if(likely(field))
    preset_field_type_size(tf, event_type, 0, 0, 
        &fixed_root, &fixed_parent,
        field);

}
#endif //0


/*****************************************************************************
 *Function name
 *    get_alignment : Get the alignment needed for a field.
 *Input params 
 *    field : field
 *
 *    returns : The size on which it must be aligned.
 *
 ****************************************************************************/
#if 0
off_t get_alignment(LttField *field)
{
  LttType *type = &field->field_type;

  switch(type->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
    case LTT_FLOAT:
    case LTT_ENUM:
      /* Align offset on type size */
      g_assert(field->field_size != 0);
      return field->field_size;
      break;
    case LTT_STRING:
      return 1;
      break;
    case LTT_ARRAY:
      g_assert(type->fields->len == 1);
      {
        LttField *child = &g_array_index(type->fields, LttField, 0);
        return get_alignment(child);
      }
      break;
    case LTT_SEQUENCE:
      g_assert(type->fields->len == 2);
      {
        off_t localign = 1;
        LttField *child = &g_array_index(type->fields, LttField, 0);

        localign = max(localign, get_alignment(child));

        child = &g_array_index(type->fields, LttField, 1);
        localign = max(localign, get_alignment(child));
        
        return localign;
      }
      break;
    case LTT_STRUCT:
    case LTT_UNION:
      {
        guint i;
        off_t localign = 1;
        
        for(i=0; i<type->fields->len; i++) {
          LttField *child = &g_array_index(type->fields, LttField, i);
          localign = max(localign, get_alignment(child));
        }
        return localign;
      }
      break;
    case LTT_NONE:
    default:
      g_error("get_alignment : unknown type");
      return -1;
  }
}

#endif //0

/*****************************************************************************
 *Function name
 *    field_compute_static_size : Determine the size of fields known by their
 *    sole definition. Unions, arrays and struct sizes might be known, but
 *    the parser does not give that information.
 *Input params 
 *    tf : tracefile
 *    field : field
 *
 ****************************************************************************/
#if 0
void field_compute_static_size(LttFacility *fac, LttField *field)
{
  LttType *type = &field->field_type;

  switch(type->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
    case LTT_FLOAT:
    case LTT_ENUM:
    case LTT_STRING:
      /* nothing to do */
      break;
    case LTT_ARRAY:
      /* note this : array type size is the number of elements in the array,
       * while array field size of the length of the array in bytes */
      g_assert(type->fields->len == 1);
      {
        LttField *child = &g_array_index(type->fields, LttField, 0);
        field_compute_static_size(fac, child);
        
        if(child->field_size != 0) {
          field->field_size = type->size * child->field_size;
          field->dynamic_offsets = g_array_sized_new(FALSE, TRUE, 
              sizeof(off_t), type->size);
        } else {
          field->field_size = 0;
        }
      }
      break;
    case LTT_SEQUENCE:
      g_assert(type->fields->len == 2);
      {
        off_t local_offset = 0;
        LttField *child = &g_array_index(type->fields, LttField, 1);
        field_compute_static_size(fac, child);
        field->field_size = 0;
        type->size = 0;
        if(child->field_size != 0) {
          field->dynamic_offsets = g_array_sized_new(FALSE, TRUE, 
              sizeof(off_t), SEQUENCE_AVG_ELEMENTS);
        }
      }
      break;
    case LTT_STRUCT:
    case LTT_UNION:
      {
        guint i;
        for(i=0;i<type->fields->len;i++) {
          LttField *child = &g_array_index(type->fields, LttField, i);
          field_compute_static_size(fac, child);
          if(child->field_size != 0) {
            type->size += ltt_align(type->size, get_alignment(child),
                                    fac->alignment);
            type->size += child->field_size;
          } else {
            /* As soon as we find a child with variable size, we have
             * a variable size */
            type->size = 0;
            break;
          }
        }
        field->field_size = type->size;
      }
      break;
    default:
      g_error("field_static_size : unknown type");
  }
      
}
#endif //0


/*****************************************************************************
 *Function name
 *    precompute_fields_offsets : set the precomputable offset of the fields
 *Input params 
 *    fac : facility
 *    field : the field
 *    offset : pointer to the current offset, must be incremented
 *
 *    return : 1 : found a variable length field, stop the processing.
 *             0 otherwise.
 ****************************************************************************/

#if 0
gint precompute_fields_offsets(LttFacility *fac, LttField *field, off_t *offset, gint is_compact)
{
  LttType *type = &field->field_type;
  
  if(unlikely(is_compact)) {
    g_assert(field->field_size != 0);
    /* FIXME THIS IS A HUUUUUGE hack :
     * offset is between the compact_data field in struct LttEvent
     * and the address of the field root in the memory map.
     * ark. Both will stay at the same addresses while the event
     * is readable, so it's ok.
     */
    field->offset_root = 0;
    field->fixed_root = FIELD_FIXED;
    return 0;
  }

  switch(type->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
    case LTT_FLOAT:
    case LTT_ENUM:
      g_assert(field->field_size != 0);
      /* Align offset on type size */
      *offset += ltt_align(*offset, get_alignment(field),
                           fac->alignment);
      /* remember offset */
      field->offset_root = *offset;
      field->fixed_root = FIELD_FIXED;
      /* Increment offset */
      *offset += field->field_size;
      return 0;
      break;
    case LTT_STRING:
      field->offset_root = *offset;
      field->fixed_root = FIELD_FIXED;
      return 1;
      break;
    case LTT_ARRAY:
      g_assert(type->fields->len == 1);
      {
        LttField *child = &g_array_index(type->fields, LttField, 0);

        *offset += ltt_align(*offset, get_alignment(field),
                             fac->alignment);
        
        /* remember offset */
        field->offset_root = *offset;
        field->array_offset = *offset;
        field->fixed_root = FIELD_FIXED;

        /* Let the child be variable */
        //precompute_fields_offsets(tf, child, offset);
      
        if(field->field_size != 0) {
          /* Increment offset */
          /* field_size is the array size in bytes */
          *offset += field->field_size;
          return 0;
        } else {
          return 1;
        }
      }
      break;
    case LTT_SEQUENCE:
      g_assert(type->fields->len == 2);
      {
        LttField *child;
        guint ret;

        *offset += ltt_align(*offset, get_alignment(field),
                             fac->alignment);

        /* remember offset */
        field->offset_root = *offset;
        field->fixed_root = FIELD_FIXED;
 
        child = &g_array_index(type->fields, LttField, 0);
        ret = precompute_fields_offsets(fac, child, offset, is_compact);
        g_assert(ret == 0); /* Seq len cannot have variable len */

        child = &g_array_index(type->fields, LttField, 1);
        *offset += ltt_align(*offset, get_alignment(child),
                             fac->alignment);
        field->array_offset = *offset;
        /* Let the child be variable. */
        //ret = precompute_fields_offsets(fac, child, offset);

        /* Cannot precompute fields offsets of sequence members, and has
         * variable length. */
        return 1;
      }
      break;
    case LTT_STRUCT:
      { 
        LttField *child;
        guint i;
        gint ret=0;

        *offset += ltt_align(*offset, get_alignment(field),
                             fac->alignment);
        /* remember offset */
        field->offset_root = *offset;
        field->fixed_root = FIELD_FIXED;

        for(i=0; i< type->fields->len; i++) {
          child = &g_array_index(type->fields, LttField, i);
          ret = precompute_fields_offsets(fac, child, offset, is_compact);

          if(ret) break;
        }
        return ret;
      }
      break;
    case LTT_UNION:
      { 
        LttField *child;
        guint i;
        gint ret=0;

        *offset += ltt_align(*offset, get_alignment(field),
                             fac->alignment);
        /* remember offset */
        field->offset_root = *offset;
        field->fixed_root = FIELD_FIXED;

        for(i=0; i< type->fields->len; i++) {
          *offset = field->offset_root;
          child = &g_array_index(type->fields, LttField, i);
          ret = precompute_fields_offsets(fac, child, offset, is_compact);

          if(ret) break;
        }
        *offset = field->offset_root + field->field_size;
        return ret;
      }

      break;
    case LTT_NONE:
    default:
      g_error("precompute_fields_offsets : unknown type");
      return 1;
  }

}

#endif //0

#if 0
/*****************************************************************************
 *Function name
 *    precompute_offsets : set the precomputable offset of an event type
 *Input params 
 *    tf : tracefile
 *    event : event type
 *
 ****************************************************************************/
void precompute_offsets(LttFacility *fac, LttEventType *event)
{
  guint i;
  off_t offset = 0;
  gint ret;

  /* First, compute the size of fixed size fields. Will determine size for
   * arrays, struct and unions, which is not done by the parser */
  for(i=0; i<event->fields->len; i++) {
    LttField *field = &g_array_index(event->fields, LttField, i);
    field_compute_static_size(fac, field);
  }
  
  /* Precompute all known offsets */
  for(i=0; i<event->fields->len; i++) {
    LttField *field = &g_array_index(event->fields, LttField, i);
    if(event->has_compact_data && i == 0)
      ret = precompute_fields_offsets(fac, field, &offset, 1);
    else
      ret = precompute_fields_offsets(fac, field, &offset, 0);
    if(ret) break;
  }
}
#endif //0



/*****************************************************************************
 *Function name
 *    preset_field_type_size : set the fixed sizes of the field type
 *Input params 
 *    tf              : tracefile
 *    event_type      : event type
 *    offset_root     : offset from the root
 *    offset_parent   : offset from the parent
 *    fixed_root      : Do we know a fixed offset to the root ?
 *    fixed_parent    : Do we know a fixed offset to the parent ?
 *    field           : field
 ****************************************************************************/



// preset the fixed size offsets. Calculate them just like genevent-new : an
// increment of a *to value that represents the offset from the start of the
// event data.
// The preset information is : offsets up to (and including) the first element
// of variable size. All subsequent fields must be flagged "VARIABLE OFFSET".
#if 0
void preset_field_type_size(LttTracefile *tf, LttEventType *event_type,
    off_t offset_root, off_t offset_parent,
    enum field_status *fixed_root, enum field_status *fixed_parent,
    LttField *field)
{
  enum field_status local_fixed_root, local_fixed_parent;
  guint i;
  LttType *type;
  
  g_assert(field->fixed_root == FIELD_UNKNOWN);
  g_assert(field->fixed_parent == FIELD_UNKNOWN);
  g_assert(field->fixed_size == FIELD_UNKNOWN);

  type = field->field_type;

  field->fixed_root = *fixed_root;
  if(field->fixed_root == FIELD_FIXED)
    field->offset_root = offset_root;
  else
    field->offset_root = 0;

  field->fixed_parent = *fixed_parent;
  if(field->fixed_parent == FIELD_FIXED)
    field->offset_parent = offset_parent;
  else
    field->offset_parent = 0;

  size_t current_root_offset;
  size_t current_offset;
  enum field_status current_child_status, final_child_status;
  size_t max_size;

  switch(type->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_FLOAT:
    case LTT_ENUM:
      field->field_size = ltt_type_size(tf->trace, type);
      field->fixed_size = FIELD_FIXED;
      break;
    case LTT_POINTER:
      field->field_size = (off_t)event_type->facility->pointer_size; 
      field->fixed_size = FIELD_FIXED;
      break;
    case LTT_LONG:
    case LTT_ULONG:
      field->field_size = (off_t)event_type->facility->long_size; 
      field->fixed_size = FIELD_FIXED;
      break;
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
      field->field_size = (off_t)event_type->facility->size_t_size; 
      field->fixed_size = FIELD_FIXED;
      break;
    case LTT_SEQUENCE:
      local_fixed_root = FIELD_VARIABLE;
      local_fixed_parent = FIELD_VARIABLE;
      preset_field_type_size(tf, event_type,
        0, 0, 
        &local_fixed_root, &local_fixed_parent,
        field->child[0]);
      field->fixed_size = FIELD_VARIABLE;
      field->field_size = 0;
      *fixed_root = FIELD_VARIABLE;
      *fixed_parent = FIELD_VARIABLE;
      break;
    case LTT_STRING:
      field->fixed_size = FIELD_VARIABLE;
      field->field_size = 0;
      *fixed_root = FIELD_VARIABLE;
      *fixed_parent = FIELD_VARIABLE;
      break;
    case LTT_ARRAY:
      local_fixed_root = FIELD_VARIABLE;
      local_fixed_parent = FIELD_VARIABLE;
      preset_field_type_size(tf, event_type,
        0, 0, 
        &local_fixed_root, &local_fixed_parent,
        field->child[0]);
      field->fixed_size = field->child[0]->fixed_size;
      if(field->fixed_size == FIELD_FIXED) {
        field->field_size = type->element_number * field->child[0]->field_size;
      } else {
        field->field_size = 0;
        *fixed_root = FIELD_VARIABLE;
        *fixed_parent = FIELD_VARIABLE;
      }
      break;
    case LTT_STRUCT:
      current_root_offset = field->offset_root;
      current_offset = 0;
      current_child_status = FIELD_FIXED;
      for(i=0;i<type->element_number;i++) {
        preset_field_type_size(tf, event_type,
          current_root_offset, current_offset, 
          fixed_root, &current_child_status,
          field->child[i]);
        if(current_child_status == FIELD_FIXED) {
          current_root_offset += field->child[i]->field_size;
          current_offset += field->child[i]->field_size;
        } else {
          current_root_offset = 0;
          current_offset = 0;
        }
      }
      if(current_child_status != FIELD_FIXED) {
        *fixed_parent = current_child_status;
        field->field_size = 0;
        field->fixed_size = current_child_status;
      } else {
        field->field_size = current_offset;
        field->fixed_size = FIELD_FIXED;
      }
      break;
    case LTT_UNION:
      current_root_offset = field->offset_root;
      current_offset = 0;
      max_size = 0;
      final_child_status = FIELD_FIXED;
      for(i=0;i<type->element_number;i++) {
        enum field_status current_root_child_status = FIELD_FIXED;
        enum field_status current_child_status = FIELD_FIXED;
        preset_field_type_size(tf, event_type,
          current_root_offset, current_offset, 
          &current_root_child_status, &current_child_status,
          field->child[i]);
        if(current_child_status != FIELD_FIXED)
          final_child_status = current_child_status;
        else
          max_size = max(max_size, field->child[i]->field_size);
      }
      if(final_child_status != FIELD_FIXED) {
        g_error("LTTV does not support variable size fields in unions.");
        /* This will stop the application. */
        *fixed_root = final_child_status;
        *fixed_parent = final_child_status;
        field->field_size = 0;
        field->fixed_size = current_child_status;
      } else {
        field->field_size = max_size;
        field->fixed_size = FIELD_FIXED;
      }
      break;
    case LTT_NONE:
      g_error("unexpected type NONE");
      break;
  }

}
#endif //0

/*****************************************************************************
 *Function name
 *    check_fields_compatibility : Check for compatibility between two fields :
 *    do they use the same inner structure ?
 *Input params 
 *    event_type1     : event type
 *    event_type2     : event type
 *    field1          : field
 *    field2          : field
 *Returns : 0 if identical
 *          1 if not.
 ****************************************************************************/
// this function checks for equality of field types. Therefore, it does not use
// per se offsets. For instance, an aligned version of a structure is
// compatible with an unaligned version of the same structure.
#if 0
gint check_fields_compatibility(LttEventType *event_type1,
    LttEventType *event_type2,
    LttField *field1, LttField *field2)
{
  guint different = 0;
  LttType *type1;
  LttType *type2;

  if(field1 == NULL) {
    if(field2 == NULL) goto end;
    else {
      different = 1;
      goto end;
    }
  } else if(field2 == NULL) {
    different = 1;
    goto end;
  }

  type1 = &field1->field_type;
  type2 = &field2->field_type;

  if(type1->type_class != type2->type_class) {
    different = 1;
    goto end;
  }
  if(type1->network != type2->network) {
    different = 1;
    goto end;
  }
 
  switch(type1->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
    case LTT_FLOAT:
    case LTT_ENUM:
      if(field1->field_size != field2->field_size)
        different = 1;
      break;
    case LTT_STRING:
      break;
    case LTT_ARRAY:
      {
        LttField *child1 = &g_array_index(type1->fields, LttField, 0);
        LttField *child2 = &g_array_index(type2->fields, LttField, 0);

        if(type1->size != type2->size)
          different = 1;
        if(check_fields_compatibility(event_type1, event_type2, child1, child2))
          different = 1;
      }
      break;
    case LTT_SEQUENCE:
      {
        LttField *child1 = &g_array_index(type1->fields, LttField, 1);
        LttField *child2 = &g_array_index(type2->fields, LttField, 1);

        if(check_fields_compatibility(event_type1, event_type2, child1, child2))
          different = 1;
      }
      break;
    case LTT_STRUCT:
    case LTT_UNION:
      { 
        LttField *child;
        guint i;
        
        if(type1->fields->len != type2->fields->len) {
          different = 1;
          goto end;
        }
        
        for(i=0; i< type1->fields->len; i++) {
          LttField *child1;
          LttField *child2;
          child1 = &g_array_index(type1->fields, LttField, i);
          child2 = &g_array_index(type2->fields, LttField, i);
          different = check_fields_compatibility(event_type1,
                        event_type2, child1, child2);

          if(different) break;
        }
      }
      break;
    case LTT_NONE:
    default:
      g_error("check_fields_compatibility : unknown type");
  }

end:
  return different;
}
#endif //0

#if 0
gint check_fields_compatibility(LttEventType *event_type1,
    LttEventType *event_type2,
    LttField *field1, LttField *field2)
{
  guint different = 0;
  guint i;
  LttType *type1;
  LttType *type2;
  
  if(field1 == NULL) {
    if(field2 == NULL) goto end;
    else {
      different = 1;
      goto end;
    }
  } else if(field2 == NULL) {
    different = 1;
    goto end;
  }
  
  g_assert(field1->fixed_root != FIELD_UNKNOWN);
  g_assert(field2->fixed_root != FIELD_UNKNOWN);
  g_assert(field1->fixed_parent != FIELD_UNKNOWN);
  g_assert(field2->fixed_parent != FIELD_UNKNOWN);
  g_assert(field1->fixed_size != FIELD_UNKNOWN);
  g_assert(field2->fixed_size != FIELD_UNKNOWN);

  type1 = field1->field_type;
  type2 = field2->field_type;

  if(type1->type_class != type2->type_class) {
    different = 1;
    goto end;
  }
  if(type1->element_name != type2->element_name) {
    different = 1;
    goto end;
  }
    
  switch(type1->type_class) {
    case LTT_INT_FIXED:
    case LTT_UINT_FIXED:
    case LTT_POINTER:
    case LTT_CHAR:
    case LTT_UCHAR:
    case LTT_SHORT:
    case LTT_USHORT:
    case LTT_INT:
    case LTT_UINT:
    case LTT_FLOAT:
    case LTT_POINTER:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
      if(field1->field_size != field2->field_size) {
        different = 1;
        goto end;
      }
      break;
    case LTT_ENUM:
      if(type1->element_number != type2->element_number) {
        different = 1;
        goto end;
      }
      for(i=0;i<type1->element_number;i++) {
        if(type1->enum_strings[i] != type2->enum_strings[i]) {
          different = 1;
          goto end;
        }
      }
      break;
    case LTT_SEQUENCE:
      /* Two elements : size and child */
      g_assert(type1->element_number != type2->element_number);
      for(i=0;i<type1->element_number;i++) {
        if(check_fields_compatibility(event_type1, event_type2,
          field1->child[0], field2->child[0])) {
          different = 1;
          goto end;
        }
      }
      break;
    case LTT_STRING:
      break;
    case LTT_ARRAY:
      if(field1->field_size != field2->field_size) {
        different = 1;
        goto end;
      }
      /* Two elements : size and child */
      g_assert(type1->element_number != type2->element_number);
      for(i=0;i<type1->element_number;i++) {
        if(check_fields_compatibility(event_type1, event_type2,
          field1->child[0], field2->child[0])) {
          different = 1;
          goto end;
        }
      }
      break;
    case LTT_STRUCT:
    case LTT_UNION:
      if(type1->element_number != type2->element_number) {
        different = 1;
        break;
      }
      for(i=0;i<type1->element_number;i++) {
        if(check_fields_compatibility(event_type1, event_type2,
          field1->child[0], field2->child[0])) {
          different = 1;
          goto end;
        }
      }
      break;
  }
end:
  return different;
}
#endif //0


/*****************************************************************************
 *Function name
 *    ltt_get_int    : get an integer number
 *Input params 
 *    reverse_byte_order: must we reverse the byte order ?
 *    size            : the size of the integer
 *    ptr             : the data pointer
 *Return value
 *    gint64          : a 64 bits integer
 ****************************************************************************/

gint64 ltt_get_int(gboolean reverse_byte_order, gint size, void *data)
{
  gint64 val;

  switch(size) {
    case 1: val = *((gint8*)data); break;
    case 2: val = ltt_get_int16(reverse_byte_order, data); break;
    case 4: val = ltt_get_int32(reverse_byte_order, data); break;
    case 8: val = ltt_get_int64(reverse_byte_order, data); break;
    default: val = ltt_get_int64(reverse_byte_order, data);
             g_critical("get_int : integer size %d unknown", size);
             break;
  }

  return val;
}

/*****************************************************************************
 *Function name
 *    ltt_get_uint    : get an unsigned integer number
 *Input params 
 *    reverse_byte_order: must we reverse the byte order ?
 *    size            : the size of the integer
 *    ptr             : the data pointer
 *Return value
 *    guint64         : a 64 bits unsigned integer
 ****************************************************************************/

guint64 ltt_get_uint(gboolean reverse_byte_order, gint size, void *data)
{
  guint64 val;

  switch(size) {
    case 1: val = *((gint8*)data); break;
    case 2: val = ltt_get_uint16(reverse_byte_order, data); break;
    case 4: val = ltt_get_uint32(reverse_byte_order, data); break;
    case 8: val = ltt_get_uint64(reverse_byte_order, data); break;
    default: val = ltt_get_uint64(reverse_byte_order, data);
             g_critical("get_uint : unsigned integer size %d unknown",
                 size);
             break;
  }

  return val;
}


/* get the node name of the system */

char * ltt_trace_system_description_node_name (LttSystemDescription * s)
{
  return s->node_name;
}


/* get the domain name of the system */

char * ltt_trace_system_description_domain_name (LttSystemDescription * s)
{
  return s->domain_name;
}


/* get the description of the system */

char * ltt_trace_system_description_description (LttSystemDescription * s)
{
  return s->description;
}


/* get the NTP corrected start time of the trace */
LttTime ltt_trace_start_time(LttTrace *t)
{
  return t->start_time;
}

/* get the monotonic start time of the trace */
LttTime ltt_trace_start_time_monotonic(LttTrace *t)
{
  return t->start_time_from_tsc;
}

static __attribute__ ((__unused__)) LttTracefile *ltt_tracefile_new()
{
  LttTracefile *tf;
  tf = g_new(LttTracefile, 1);
  tf->event.tracefile = tf;
  return tf;
}

static __attribute__ ((__unused__)) void ltt_tracefile_destroy(LttTracefile *tf)
{
  g_free(tf);
}

static __attribute__ ((__unused__)) void ltt_tracefile_copy(LttTracefile *dest, const LttTracefile *src)
{
  *dest = *src;
}

/* Before library loading... */

static __attribute__((constructor)) void init(void)
{
  LTT_TRACEFILE_NAME_METADATA = g_quark_from_string("metadata");
}
