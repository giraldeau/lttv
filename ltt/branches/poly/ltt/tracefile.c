/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Mathieu Desnoyers
 *
 * Complete rewrite from the original version made by XangXiu Yang.
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
#include <malloc.h>
#include <sys/mman.h>

// For realpath
#include <limits.h>
#include <stdlib.h>


#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/ltt-types.h>


/* Facility names used in this file */

GQuark LTT_FACILITY_NAME_HEARTBEAT,
       LTT_EVENT_NAME_HEARTBEAT;
GQuark LTT_TRACEFILE_NAME_FACILITIES;

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

/* obtain the time of an event */

static inline LttTime getEventTime(LttTracefile * tf);


/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void set_fields_offsets(LttTracefile *tf, LttEventType *event_type);
//size_t get_fields_offsets(LttTracefile *tf, LttEventType *event_type, void *data);

/* get the size of the field type according to 
 * The facility size information. */
static inline void preset_field_type_size(LttTracefile *tf,
    LttEventType *event_type,
    off_t offset_root, off_t offset_parent,
    enum field_status *fixed_root, enum field_status *fixed_parent,
    LttField *field);

/* map a fixed size or a block information from the file (fd) */
static gint map_block(LttTracefile * tf, guint block_num);

/* calculate nsec per cycles for current block */
static double calc_nsecs_per_cycle(LttTracefile * t);

/* go to the next event */
static int ltt_seek_next_event(LttTracefile *tf);

void ltt_update_event_size(LttTracefile *tf);

#if 0
/* Functions to parse system.xml file (using glib xml parser) */
static void parser_start_element (GMarkupParseContext  __UNUSED__ *context,
				  const gchar          *element_name,
				  const gchar         **attribute_names,
				  const gchar         **attribute_values,
				  gpointer              user_data,
				  GError              **error)
{
  int i=0;
  LttSystemDescription* des = (LttSystemDescription* )user_data;
  if(strcmp("system", element_name)){
    *error = g_error_new(G_MARKUP_ERROR,
                         G_LOG_LEVEL_WARNING,
                         "This is not system.xml file");
    return;
  }
  
  while(attribute_names[i]){
    if(strcmp("node_name", attribute_names[i])==0){
	des->node_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("domainname", attribute_names[i])==0){
	des->domain_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("cpu", attribute_names[i])==0){
	des->nb_cpu = atoi(attribute_values[i]);      
    }else if(strcmp("arch_size", attribute_names[i])==0){
	if(strcmp(attribute_values[i],"LP32") == 0) des->size = LTT_LP32;
	else if(strcmp(attribute_values[i],"ILP32") == 0) des->size = LTT_ILP32;
	else if(strcmp(attribute_values[i],"LP64") == 0) des->size = LTT_LP64;
	else if(strcmp(attribute_values[i],"ILP64") == 0) des->size = LTT_ILP64;
	else if(strcmp(attribute_values[i],"UNKNOWN") == 0) des->size = LTT_UNKNOWN;
    }else if(strcmp("endian", attribute_names[i])==0){
	if(strcmp(attribute_values[i],"LITTLE_ENDIAN") == 0)
	  des->endian = LTT_LITTLE_ENDIAN;
	else if(strcmp(attribute_values[i],"BIG_ENDIAN") == 0) 
	  des->endian = LTT_BIG_ENDIAN;
    }else if(strcmp("kernel_name", attribute_names[i])==0){
	des->kernel_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("kernel_release", attribute_names[i])==0){
	des->kernel_release = g_strdup(attribute_values[i]);      
    }else if(strcmp("kernel_version", attribute_names[i])==0){
	des->kernel_version = g_strdup(attribute_values[i]);      
    }else if(strcmp("machine", attribute_names[i])==0){
	des->machine = g_strdup(attribute_values[i]);      
    }else if(strcmp("processor", attribute_names[i])==0){
	des->processor = g_strdup(attribute_values[i]);      
    }else if(strcmp("hardware_platform", attribute_names[i])==0){
	des->hardware_platform = g_strdup(attribute_values[i]);      
    }else if(strcmp("operating_system", attribute_names[i])==0){
	des->operating_system = g_strdup(attribute_values[i]);      
    }else if(strcmp("ltt_major_version", attribute_names[i])==0){
	des->ltt_major_version = atoi(attribute_values[i]);      
    }else if(strcmp("ltt_minor_version", attribute_names[i])==0){
	des->ltt_minor_version = atoi(attribute_values[i]);      
    }else if(strcmp("ltt_block_size", attribute_names[i])==0){
	des->ltt_block_size = atoi(attribute_values[i]);      
    }else{
      *error = g_error_new(G_MARKUP_ERROR,
                           G_LOG_LEVEL_WARNING,
                           "Not a valid attribute");
      return;      
    }
    i++;
  }
}

static void  parser_characters   (GMarkupParseContext __UNUSED__ *context,
				  const gchar          *text,
				  gsize __UNUSED__      text_len,
				  gpointer              user_data,
				  GError __UNUSED__     **error)
{
  LttSystemDescription* des = (LttSystemDescription* )user_data;
  des->description = g_strdup(text);
}
#endif //0
LttFacility *ltt_trace_get_facility_by_num(LttTrace *t,
    guint num)
{
  g_assert(num < t->facilities_by_num->len);
  
  return &g_array_index(t->facilities_by_num, LttFacility, num);

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

gint ltt_tracefile_open(LttTrace *t, gchar * fileName, LttTracefile *tf)
{
  struct stat    lTDFStat;    /* Trace data file status */
  struct ltt_block_start_header *header;
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
  if(lTDFStat.st_size < (off_t)(sizeof(struct ltt_block_start_header))){
    g_print("The input data file %s does not contain a trace\n", fileName);
    goto close_file;
  }
  
  /* Temporarily map the buffer start header to get trace information */
  /* Multiple of pages aligned head */
  tf->buffer.head = mmap(0,
      PAGE_ALIGN(sizeof(struct ltt_block_start_header)), PROT_READ, 
      MAP_PRIVATE, tf->fd, 0);
  if(tf->buffer.head == MAP_FAILED) {
    perror("Error in allocating memory for buffer of tracefile");
    goto close_file;
  }
  g_assert( ( (guint)tf->buffer.head&(8-1) ) == 0); // make sure it's aligned.
  
  header = (struct ltt_block_start_header*)tf->buffer.head;
  
  if(header->trace.magic_number == LTT_MAGIC_NUMBER)
    tf->reverse_bo = 0;
  else if(header->trace.magic_number == LTT_REV_MAGIC_NUMBER)
    tf->reverse_bo = 1;
  else  /* invalid magic number, bad tracefile ! */
    goto unmap_file;
    
  //store the size of the file
  tf->file_size = lTDFStat.st_size;
  tf->buf_size = ltt_get_uint32(LTT_GET_BO(tf), &header->buf_size);
  tf->num_blocks = tf->file_size / tf->buf_size;

  if(munmap(tf->buffer.head,
        PAGE_ALIGN(sizeof(struct ltt_block_start_header)))) {
    g_warning("unmap size : %u\n",
        PAGE_ALIGN(sizeof(struct ltt_block_start_header)));
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
        PAGE_ALIGN(sizeof(struct ltt_block_start_header)))) {
    g_warning("unmap size : %u\n",
        PAGE_ALIGN(sizeof(struct ltt_block_start_header)));
    perror("munmap error");
    g_assert(0);
  }
close_file:
  close(tf->fd);
end:
  return -1;
}

LttTrace *ltt_tracefile_get_trace(LttTracefile *tf)
{
  return tf->trace;
}

#if 0
/*****************************************************************************
 *Open control and per cpu tracefiles
 ****************************************************************************/

void ltt_tracefile_open_cpu(LttTrace *t, gchar * tracefile_name)
{
  LttTracefile * tf;
  tf = ltt_tracefile_open(t,tracefile_name);
  if(!tf) return;
  t->per_cpu_tracefile_number++;
  g_ptr_array_add(t->per_cpu_tracefiles, tf);
}

gint ltt_tracefile_open_control(LttTrace *t, gchar * control_name)
{
  LttTracefile * tf;
  LttEvent ev;
  LttFacility * f;
  void * pos;
  FacilityLoad fLoad;
  unsigned int i;

  tf = ltt_tracefile_open(t,control_name);
  if(!tf) {
	  g_warning("ltt_tracefile_open_control : bad file descriptor");
    return -1;
  }
  t->control_tracefile_number++;
  g_ptr_array_add(t->control_tracefiles,tf);

  //parse facilities tracefile to get base_id
  if(strcmp(&control_name[strlen(control_name)-10],"facilities") ==0){
    while(1){
      if(!ltt_tracefile_read(tf,&ev)) return 0; // end of file

      if(ev.event_id == TRACE_FACILITY_LOAD){
	pos = ev.data;
	fLoad.name = (gchar*)pos;
	fLoad.checksum = *(LttChecksum*)(pos + strlen(fLoad.name));
	fLoad.base_code = *(guint32 *)(pos + strlen(fLoad.name) + sizeof(LttChecksum));

	for(i=0;i<t->facility_number;i++){
	  f = (LttFacility*)g_ptr_array_index(t->facilities,i);
	  if(strcmp(f->name,fLoad.name)==0 && fLoad.checksum==f->checksum){
	    f->base_id = fLoad.base_code;
	    break;
	  }
	}
	if(i==t->facility_number) {
	  g_warning("Facility: %s, checksum: %u is not found",
		  fLoad.name,(unsigned int)fLoad.checksum);
    return -1;
  }
      }else if(ev.event_id == TRACE_BLOCK_START){
	continue;
      }else if(ev.event_id == TRACE_BLOCK_END){
	break;
      }else {
        g_warning("Not valid facilities trace file");
        return -1;
      }
    }
  }
  return 0;
}
#endif //0

/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, 
 *Input params
 *    t                  : tracefile which will be closed
 ****************************************************************************/

void ltt_tracefile_close(LttTracefile *t)
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


/*****************************************************************************
 *Get system information
 ****************************************************************************/
#if 0
gint getSystemInfo(LttSystemDescription* des, gchar * pathname)
{
  int fd;
  GIOChannel *iochan;
  gchar *buf = NULL;
  gsize length;

  GMarkupParseContext * context;
  GError * error = NULL;
  GMarkupParser markup_parser =
    {
      parser_start_element,
      NULL,
      parser_characters,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  fd = g_open(pathname, O_RDONLY, 0);
  if(fd == -1){
    g_warning("Can not open file : %s\n", pathname);
    return -1;
  }
  
  iochan = g_io_channel_unix_new(fd);
  
  context = g_markup_parse_context_new(&markup_parser, 0, des,NULL);
  
  //while(fgets(buf,DIR_NAME_SIZE, fp) != NULL){
  while(g_io_channel_read_line(iochan, &buf, &length, NULL, &error)
      != G_IO_STATUS_EOF) {

    if(error != NULL) {
      g_warning("Can not read xml file: \n%s\n", error->message);
      g_error_free(error);
    }
    if(!g_markup_parse_context_parse(context, buf, length, &error)){
      if(error != NULL) {
        g_warning("Can not parse xml file: \n%s\n", error->message);
        g_error_free(error);
      }
      g_markup_parse_context_free(context);

      g_io_channel_shutdown(iochan, FALSE, &error); /* No flush */
      if(error != NULL) {
        g_warning("Can not close file: \n%s\n", error->message);
        g_error_free(error);
      }

      close(fd);
      return -1;
    }
  }
  g_markup_parse_context_free(context);

  g_io_channel_shutdown(iochan, FALSE, &error); /* No flush */
  if(error != NULL) {
    g_warning("Can not close file: \n%s\n", error->message);
    g_error_free(error);
  }

  g_close(fd);

  g_free(buf);
  return 0;
}
#endif //0

/*****************************************************************************
 *The following functions get facility/tracefile information
 ****************************************************************************/
#if 0
gint getFacilityInfo(LttTrace *t, gchar* eventdefs)
{
  GDir * dir;
  const gchar * name;
  unsigned int i,j;
  LttFacility * f;
  LttEventType * et;
  gchar fullname[DIR_NAME_SIZE];
  GError * error = NULL;

  dir = g_dir_open(eventdefs, 0, &error);

  if(error != NULL) {
    g_warning("Can not open directory: %s, %s\n", eventdefs, error->message);
    g_error_free(error);
    return -1;
  }

  while((name = g_dir_read_name(dir)) != NULL){
    if(!g_pattern_match_simple("*.xml", name)) continue;
    strcpy(fullname,eventdefs);
    strcat(fullname,name);
    ltt_facility_open(t,fullname);
  }
  g_dir_close(dir);
  
  for(j=0;j<t->facility_number;j++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, j);
    for(i=0; i<f->event_number; i++){
      et = f->events[i];
      setFieldsOffset(NULL, et, NULL, t);
    }    
  }
  return 0;
}
#endif //0

/*****************************************************************************
 *A trace is specified as a pathname to the directory containing all the
 *associated data (control tracefiles, per cpu tracefiles, event 
 *descriptions...).
 *
 *When a trace is closed, all the associated facilities, types and fields
 *are released as well.
 */


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

  if ( realpath (pathname, abs_pathname) != NULL)
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
 */

int get_tracefile_name_number(const gchar *raw_name,
                              GQuark *name,
                              guint *num)
{
  guint raw_name_len = strlen(raw_name);
  gchar char_name[PATH_MAX];
  gchar *digit_begin;
  int i;
  int underscore_pos;
  long int cpu_num;
  gchar *endptr;

  for(i=raw_name_len-1;i>=0;i--) {
    if(raw_name[i] == '_') break;
  }
  if(i==0)  /* Either not found or name length is 0 */
    return -1;
  underscore_pos = i;

  cpu_num = strtol(raw_name+underscore_pos+1, &endptr, 10);

  if(endptr == raw_name+underscore_pos+1)
    return -1; /* No digit */
  if(cpu_num == LONG_MIN || cpu_num == LONG_MAX)
    return -1; /* underflow / overflow */
  
  strncpy(char_name, raw_name, underscore_pos);
  
  char_name[underscore_pos] = '\0';
  
  *name = g_quark_from_string(char_name);
  *num = cpu_num;
  
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
  int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      args->func(tf, args->func_args);
  }
}


void ltt_tracefile_group_destroy(gpointer data)
{
  GArray *group = (GArray *)data;
  int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      ltt_tracefile_close(tf);
  }
  g_array_free(group, TRUE);
}

gboolean ltt_tracefile_group_has_cpu_online(gpointer data)
{
  GArray *group = (GArray *)data;
  int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online) return 1;
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
 * A tracefile group is simply an array where all the per cpu tracefiles sits.
 */

static int open_tracefiles(LttTrace *trace, gchar *root_path,
    gchar *relative_path)
{
	DIR *dir = opendir(root_path);
	struct dirent *entry;
	struct stat stat_buf;
	int ret;
  
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
		
		if(S_ISDIR(stat_buf.st_mode)) {

			g_debug("Entering subdirectory...\n");
			ret = open_tracefiles(trace, path, rel_path);
			if(ret < 0) continue;
		} else if(S_ISREG(stat_buf.st_mode)) {
			GQuark name;
      guint num;
      GArray *group;
      LttTracefile *tf;
      guint len;
      
      if(get_tracefile_name_number(rel_path, &name, &num))
        continue; /* invalid name */
      
			g_debug("Opening file.\n");
      if(ltt_tracefile_open(trace, path, &tmp_tf)) {
        g_info("Error opening tracefile %s", path);

        continue; /* error opening the tracefile : bad magic number ? */
      }

      g_debug("Tracefile name is %s and number is %u", 
          g_quark_to_string(name), num);
      
      tmp_tf.cpu_online = 1;
      tmp_tf.cpu_num = num;
      tmp_tf.name = name;

      group = g_datalist_id_get_data(&trace->tracefiles, name);
      if(group == NULL) {
        /* Elements are automatically cleared when the array is allocated.
         * It makes the cpu_online variable set to 0 : cpu offline, by default.
         */
        group = g_array_sized_new (FALSE, TRUE, sizeof(LttTracefile), 10);
        g_datalist_id_set_data_full(&trace->tracefiles, name,
                                 group, ltt_tracefile_group_destroy);
      }

      /* Add the per cpu tracefile to the named group */
      unsigned int old_len = group->len;
      if(num+1 > old_len)
        group = g_array_set_size(group, num+1);
      g_array_index (group, LttTracefile, num) = tmp_tf;

		}
	}
	
	closedir(dir);

	return 0;
}

/* ltt_get_facility_description
 *
 * Opens the trace corresponding to the requested facility (identified by fac_id
 * and checksum).
 *
 * The name searched is : %trace root%/eventdefs/facname_checksum.xml
 *
 * Returns 0 on success, or 1 on failure.
 */

static int ltt_get_facility_description(LttFacility *f, 
                                        LttTrace *t,
                                        LttTracefile *fac_tf)
{
  char desc_file_name[PATH_MAX];
  const gchar *text;
  guint textlen;
  gint err;

  text = g_quark_to_string(t->pathname);
  textlen = strlen(text);
  
  if(textlen >= PATH_MAX) goto name_error;
  strcpy(desc_file_name, text);

  text = "/eventdefs/";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);
  
  text = g_quark_to_string(f->name);
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);

  text = "_";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);

  err = snprintf(desc_file_name+textlen, PATH_MAX-textlen-1,
      "%u", f->checksum);
  if(err < 0) goto name_error;

  textlen=strlen(desc_file_name);
  
  text = ".xml";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);
 
  err = ltt_facility_open(f, t, desc_file_name);
  if(err) goto facility_error;

  return 0;

facility_error:
name_error:
  return 1;
}

static void ltt_fac_ids_destroy(gpointer data)
{
  GArray *fac_ids = (GArray *)data;

  g_array_free(fac_ids, TRUE);
}


/* Presumes the tracefile is already seeked at the beginning. It makes sense,
 * because it must be done just after the opening */
static int ltt_process_facility_tracefile(LttTracefile *tf)
{
  int err;
  LttFacility *fac;
  GArray *fac_ids;
  guint i;
  LttEventType *et;
  
  while(1) {
    err = ltt_tracefile_read_seek(tf);
    if(err == EPERM) goto seek_error;
    else if(err == ERANGE) break; /* End of tracefile */

    err = ltt_tracefile_read_update_event(tf);
    if(err) goto update_error;

    /* We are on a facility load/or facility unload/ or heartbeat event */
    /* The rules are :
     * * facility 0 is hardcoded : this is the core facility. It will be shown
     *   in the facility array though, and is shown as "loaded builtin" in the
     *   trace.
     * It contains event :
     *  0 : facility load
     *  1 : facility unload
     *  2 : state dump facility load
     *  3 : heartbeat
     */
    if(tf->event.facility_id != LTT_FACILITY_CORE) {
      /* Should only contain core facility */
      g_warning("Error in processing facility file %s, "
          "should not contain facility id  %u.", g_quark_to_string(tf->name),
          tf->event.facility_id);
      err = EPERM;
      goto fac_id_error;
    } else {
    
      struct LttFacilityLoad *fac_load_data;
      struct LttStateDumpFacilityLoad *fac_state_dump_load_data;
      char *fac_name;

      // FIXME align
      switch((enum ltt_core_events)tf->event.event_id) {
        case LTT_EVENT_FACILITY_LOAD:
          fac_name = (char*)(tf->event.data);
          g_debug("Doing LTT_EVENT_FACILITY_LOAD of facility %s",
              fac_name);
          fac_load_data =
            (struct LttFacilityLoad *)
                (tf->event.data + strlen(fac_name) + 1);
          fac = &g_array_index (tf->trace->facilities_by_num, LttFacility,
              ltt_get_uint32(LTT_GET_BO(tf), &fac_load_data->id));
          g_assert(fac->exists == 0);
          fac->name = g_quark_from_string(fac_name);
          fac->checksum = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_load_data->checksum);
          fac->id = ltt_get_uint32(LTT_GET_BO(tf), &fac_load_data->id);
          fac->pointer_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_load_data->pointer_size);
          fac->long_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_load_data->long_size);
          fac->size_t_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_load_data->size_t_size);
          fac->alignment = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_load_data->alignment);

          if(ltt_get_facility_description(fac, tf->trace, tf))
            continue; /* error opening description */
          
          fac->trace = tf->trace;

          /* Preset the field offsets */
          for(i=0; i<fac->events->len; i++){
            et = &g_array_index(fac->events, LttEventType, i);
            set_fields_offsets(tf, et);
          }

          fac->exists = 1;

          fac_ids = g_datalist_id_get_data(&tf->trace->facilities_by_name,
                                fac->name);
          if(fac_ids == NULL) {
            fac_ids = g_array_sized_new (FALSE, TRUE, sizeof(guint), 1);
            g_datalist_id_set_data_full(&tf->trace->facilities_by_name,
                                     fac->name,
                                     fac_ids, ltt_fac_ids_destroy);
          }
          g_array_append_val(fac_ids, fac->id);

          break;
        case LTT_EVENT_FACILITY_UNLOAD:
          g_debug("Doing LTT_EVENT_FACILITY_UNLOAD");
          /* We don't care about unload : facilities ID are valid for the whole
           * trace. They simply won't be used after the unload. */
          break;
        case LTT_EVENT_STATE_DUMP_FACILITY_LOAD:
          fac_name = (char*)(tf->event.data);
          g_debug("Doing LTT_EVENT_STATE_DUMP_FACILITY_LOAD of facility %s",
              fac_name);
          fac_state_dump_load_data =
            (struct LttStateDumpFacilityLoad *)
                (tf->event.data + strlen(fac_name) + 1);
          fac = &g_array_index (tf->trace->facilities_by_num, LttFacility,
              ltt_get_uint32(LTT_GET_BO(tf), &fac_state_dump_load_data->id));
          g_assert(fac->exists == 0);
          fac->name = g_quark_from_string(fac_name);
          fac->checksum = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_state_dump_load_data->checksum);
          fac->id = fac_state_dump_load_data->id;
          fac->pointer_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_state_dump_load_data->pointer_size);
          fac->long_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_state_dump_load_data->long_size);
          fac->size_t_size = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_state_dump_load_data->size_t_size);
          fac->alignment = ltt_get_uint32(LTT_GET_BO(tf),
                          &fac_state_dump_load_data->alignment);
          if(ltt_get_facility_description(fac, tf->trace, tf))
            continue; /* error opening description */
          
          fac->trace = tf->trace;

          /* Preset the field offsets */
          for(i=0; i<fac->events->len; i++){
            et = &g_array_index(fac->events, LttEventType, i);
            set_fields_offsets(tf, et);
          }

          fac->exists = 1;
          
          fac_ids = g_datalist_id_get_data(&tf->trace->facilities_by_name,
              fac->name);
          if(fac_ids == NULL) {
            fac_ids = g_array_sized_new (FALSE, TRUE, sizeof(guint), 1);
            g_datalist_id_set_data_full(&tf->trace->facilities_by_name,
                                     fac->name,
                                     fac_ids, ltt_fac_ids_destroy);
          }
          g_array_append_val(fac_ids, fac->id);

          break;
        case LTT_EVENT_HEARTBEAT:
          break;
        default:
          g_warning("Error in processing facility file %s, "
              "unknown event id %hhu in core facility.",
              g_quark_to_string(tf->name),
              tf->event.event_id);
          err = EPERM;
          goto event_id_error;
      }
    }
  }
  return 0;

  /* Error handling */
facility_error:
event_id_error:
fac_id_error:
update_error:
seek_error:
  g_warning("An error occured in facility tracefile parsing");
  return err;
}


LttTrace *ltt_trace_open(const gchar *pathname)
{
  gchar abs_path[PATH_MAX];
  LttTrace  * t;
  LttTracefile *tf;
  GArray *group;
  int i;
  struct ltt_block_start_header *header;
  
  t = g_new(LttTrace, 1);
  if(!t) goto alloc_error;

  get_absolute_pathname(pathname, abs_path);
  t->pathname = g_quark_from_string(abs_path);

  /* Open all the tracefiles */
  g_datalist_init(&t->tracefiles);
  if(open_tracefiles(t, abs_path, "")) {
    g_warning("Error opening tracefile %s", abs_path);
    goto open_error;
  }
  
  /* Prepare the facilities containers : array and mapping */
  /* Array is zeroed : the "exists" field is set to false by default */
  t->facilities_by_num = g_array_sized_new (FALSE, 
                                            TRUE, sizeof(LttFacility),
                                            NUM_FACILITIES);
  t->facilities_by_num = g_array_set_size(t->facilities_by_num, NUM_FACILITIES);

  g_datalist_init(&t->facilities_by_name);
  
  /* Parse each trace control/facilitiesN files : get runtime fac. info */
  group = g_datalist_id_get_data(&t->tracefiles, LTT_TRACEFILE_NAME_FACILITIES);
  if(group == NULL) {
    g_error("Trace %s has no facility tracefile", abs_path);
    g_assert(0);
    goto facilities_error;
  }

  /* Get the trace information for the control/facility 0 tracefile */
  g_assert(group->len > 0);
  tf = &g_array_index (group, LttTracefile, 0);
  header = (struct ltt_block_start_header*)tf->buffer.head;
  t->arch_type = ltt_get_uint32(LTT_GET_BO(tf), &header->trace.arch_type);
  t->arch_variant = ltt_get_uint32(LTT_GET_BO(tf), &header->trace.arch_variant);
  t->arch_size = header->trace.arch_size;
  t->ltt_major_version = header->trace.major_version;
  t->ltt_minor_version = header->trace.minor_version;
  t->flight_recorder = header->trace.flight_recorder;
  t->has_heartbeat = header->trace.has_heartbeat;
  t->has_alignment = header->trace.has_alignment;
  t->has_tsc = header->trace.has_tsc;
  
  
  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(ltt_process_facility_tracefile(tf))
      goto facilities_error;
  }
  
  return t;

  /* Error handling */
facilities_error:
  g_datalist_clear(&t->facilities_by_name);
  g_array_free(t->facilities_by_num, TRUE);
open_error:
  g_datalist_clear(&t->tracefiles);
  g_free(t);
alloc_error:
  return NULL;

}

GQuark ltt_trace_name(LttTrace *t)
{
  return t->pathname;
}


/******************************************************************************
 * When we copy a trace, we want all the opening actions to happen again :
 * the trace will be reopened and totally independant from the original.
 * That's why we call ltt_trace_open.
 *****************************************************************************/
LttTrace *ltt_trace_copy(LttTrace *self)
{
  return ltt_trace_open(g_quark_to_string(self->pathname));
}

void ltt_trace_close(LttTrace *t)
{
  guint i;
  LttFacility *fac;

  for(i=0; i<t->facilities_by_num->len; i++) {
    fac = &g_array_index (t->facilities_by_num, LttFacility, i);
    if(fac->exists)
      ltt_facility_close(fac);
  }

  g_datalist_clear(&t->facilities_by_name);
  g_array_free(t->facilities_by_num, TRUE);
  g_datalist_clear(&t->tracefiles);
  g_free(t);
}


/*****************************************************************************
 *Get the system description of the trace
 ****************************************************************************/

LttFacility *ltt_trace_facility_by_id(LttTrace *t, guint8 id)
{
  g_assert(id < t->facilities_by_num->len);
  return &g_array_index(t->facilities_by_num, LttFacility, id);
}

/* ltt_trace_facility_get_by_name
 *
 * Returns the GArray of facility indexes. All the fac_ids that matches the
 * requested facility name.
 *
 * If name is not found, returns NULL.
 */
GArray *ltt_trace_facility_get_by_name(LttTrace *t, GQuark name)
{
  return g_datalist_id_get_data(&t->facilities_by_name, name);
}

/*****************************************************************************
 * Functions to discover all the event types in the trace 
 ****************************************************************************/

#if 0
unsigned ltt_trace_eventtype_number(LttTrace *t)
{
  unsigned int i;
  unsigned count = 0;
  unsigned int num = t->facility_number;
  LttFacility * f;
  
  for(i=0;i<num;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    count += f->event_number;
  }
  return count;
}
#endif //0

#if 0
//use an iteration on all the trace facilities, and inside iteration on all the
//event types in each facilities instead.
LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned evId)
{
  LttEventType *event_type;
  
  LttFacility * f;
  f = ltt_trace_facility_by_id(t,evId);

  if(unlikely(!f)) event_type = NULL;
  else event_type = f->events[evId - f->base_id];

  return event_type;
}
#endif //0

#if 0
/*****************************************************************************
 * ltt_trace_find_tracefile
 *
 * Find a tracefile by name and index in the group.
 *
 * Returns a pointer to the tracefiles, else NULL.
 ****************************************************************************/

LttTracefile *ltt_trace_find_tracefile(LttTrace *t, const gchar *name)
{
}
#endif //0

/*****************************************************************************
 * Get the start time and end time of the trace 
 ****************************************************************************/

static void ltt_tracefile_time_span_get(LttTracefile *tf,
                                        LttTime *start, LttTime *end)
{
  struct ltt_block_start_header * header;
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
  int i;
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

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end)
{
  LttTime min_start = ltt_time_infinite;
  LttTime max_end = ltt_time_zero;
  struct tracefile_time_span_get_args args = { t, &min_start, &max_end };

  g_datalist_foreach(&t->tracefiles, &group_time_span_get, &args);
  
  if(start != NULL) *start = min_start;
  if(end != NULL) *end = max_end;
  
}


/*****************************************************************************
 *Get the name of a tracefile
 ****************************************************************************/

GQuark ltt_tracefile_name(LttTracefile *tf)
{
  return tf->name;
}


guint ltt_tracefile_num(LttTracefile *tf)
{
  return tf->cpu_num;
}

/*****************************************************************************
 * Get the number of blocks in the tracefile 
 ****************************************************************************/

guint ltt_tracefile_block_number(LttTracefile *tf)
{
  return tf->num_blocks; 
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

        if(ltt_time_compare(time, tf->event.event_time) >= 0)
          goto found;
      }

    } else if(ltt_time_compare(time, tf->buffer.begin.timestamp) < 0) {
      /* go to lower part */
      high = block_num;
    } else if(ltt_time_compare(time, tf->buffer.end.timestamp) > 0) {
      /* go to higher part */
      low = block_num;
    } else {/* The event is right in the buffer!
               (or in the next buffer first event) */
      while(1) {
        ret = ltt_tracefile_read(tf);
        if(ret == ERANGE) goto range; /* ERANGE or EPERM */
        else if(ret) goto fail;

        if(ltt_time_compare(time, tf->event.event_time) >= 0)
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


int ltt_tracefile_seek_position(LttTracefile *tf, const LttEventPosition *ep) {
  
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

  err = ltt_tracefile_read_update_event(tf);
  if(err) goto fail;
  err = ltt_tracefile_read_op(tf);
  if(err) goto fail;

  return;

fail:
  g_error("ltt_tracefile_seek_time failed on tracefile %s", 
      g_quark_to_string(tf->name));
}

/* Calculate the real event time based on the buffer boundaries */
LttTime ltt_interpolate_time(LttTracefile *tf, LttEvent *event)
{
  LttTime time;

  g_assert(tf->trace->has_tsc);

  time = ltt_time_from_uint64(
      (guint64)(tf->buffer.tsc - tf->buffer.begin.cycle_count) * 
                                          tf->buffer.nsecs_per_cycle);
  time = ltt_time_add(tf->buffer.begin.timestamp, time);

  return time;
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
  err = ltt_tracefile_read_op(tf);
  if(err) return err;

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


/* do specific operation on events */
int ltt_tracefile_read_op(LttTracefile *tf)
{
  int err;
  LttFacility *f;
  void * pos;
  LttEvent *event;

  event = &tf->event;

   /* do event specific operation */

  /* do something if its an heartbeat event : increment the heartbeat count */
  //if(event->facility_id == LTT_FACILITY_CORE)
  //  if(event->event_id == LTT_EVENT_HEARTBEAT)
  //    tf->cur_heart_beat_number++;
  
  return 0;
}


/* same as ltt_tracefile_read, but does not seek to the next event nor call
 * event specific operation. */
int ltt_tracefile_read_update_event(LttTracefile *tf)
{
  int err;
  LttFacility *f;
  void * pos;
  LttEvent *event;
 
  event = &tf->event;
  pos = tf->buffer.head + event->offset;

  /* Read event header */
  
  //TODO align
  
  if(tf->trace->has_tsc) {
    if(tf->trace->has_heartbeat) {
      event->time.timestamp = ltt_get_uint32(LTT_GET_BO(tf),
                                            pos);
      /* 32 bits -> 64 bits tsc */
      /* note : still works for seek and non seek cases. */
      if(event->time.timestamp < (0xFFFFFFFFULL&tf->buffer.tsc)) {
        tf->buffer.tsc = ((tf->buffer.tsc&0xFFFFFFFF00000000ULL)
                            + 0x100000000ULL)
                                | (guint64)event->time.timestamp;
        event->tsc = tf->buffer.tsc;
      } else {
        /* no overflow */
        tf->buffer.tsc = (tf->buffer.tsc&0xFFFFFFFF00000000ULL) 
                                | (guint64)event->time.timestamp;
        event->tsc = tf->buffer.tsc;
      }
      pos += sizeof(guint32);
    } else {
      event->tsc = ltt_get_uint64(LTT_GET_BO(tf), pos);
      tf->buffer.tsc = event->tsc;
      pos += sizeof(guint64);
    }
    
    event->event_time = ltt_interpolate_time(tf, event);
  } else {
    event->time.delta.tv_sec = 0;
    event->time.delta.tv_nsec = ltt_get_uint32(LTT_GET_BO(tf),
                                          pos) * NSEC_PER_USEC;
    tf->buffer.tsc = 0;
    event->tsc = tf->buffer.tsc;

    event->event_time = ltt_time_add(tf->buffer.begin.timestamp,
                                     event->time.delta);
    pos += sizeof(guint32);
  }

  event->facility_id = *(guint8*)pos;
  pos += sizeof(guint8);

  event->event_id = *(guint8*)pos;
  pos += sizeof(guint8);

  event->event_size = ltt_get_uint16(LTT_GET_BO(tf), pos);
  pos += sizeof(guint16);
  
  event->data = pos;

  /* get the data size and update the event fields with the current
   * information */
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
  struct ltt_block_start_header *header;

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
  g_assert( ( (guint)tf->buffer.head&(8-1) ) == 0); // make sure it's aligned.
  

  tf->buffer.index = block_num;

  header = (struct ltt_block_start_header*)tf->buffer.head;

  tf->buffer.begin.timestamp = ltt_get_time(LTT_GET_BO(tf),
                                              &header->begin.timestamp);
  tf->buffer.begin.timestamp.tv_nsec *= NSEC_PER_USEC;
  g_debug("block %u begin : %lu.%lu", block_num,
      tf->buffer.begin.timestamp.tv_sec, tf->buffer.begin.timestamp.tv_nsec);
  tf->buffer.begin.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              &header->begin.cycle_count);
  tf->buffer.end.timestamp = ltt_get_time(LTT_GET_BO(tf),
                                              &header->end.timestamp);
  tf->buffer.end.timestamp.tv_nsec *= NSEC_PER_USEC;
  g_debug("block %u end : %lu.%lu", block_num,
      tf->buffer.end.timestamp.tv_sec, tf->buffer.end.timestamp.tv_nsec);
  tf->buffer.end.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              &header->end.cycle_count);
  tf->buffer.lost_size = ltt_get_uint32(LTT_GET_BO(tf),
                                              &header->lost_size);
  
  tf->buffer.tsc =  tf->buffer.begin.cycle_count;
  tf->event.tsc = tf->buffer.tsc;

  /* FIXME
   * eventually support variable buffer size : will need a partial pre-read of
   * the headers to create an index when we open the trace... eventually. */
  g_assert(tf->buf_size  == ltt_get_uint32(LTT_GET_BO(tf), 
                                             &header->buf_size));
  
  /* Now that the buffer is mapped, calculate the time interpolation for the
   * block. */
  
  tf->buffer.nsecs_per_cycle = calc_nsecs_per_cycle(tf);
 
  /* Make the current event point to the beginning of the buffer :
   * it means that the event read must get the first event. */
  tf->event.tracefile = tf;
  tf->event.block = block_num;
  tf->event.offset = 0;
  
  return 0;

map_error:
  return -errno;

}

/* It will update the fields offsets too */
void ltt_update_event_size(LttTracefile *tf)
{
  ssize_t size = 0;

  /* Specific handling of core events : necessary to read the facility control
   * tracefile. */
  LttFacility *f = ltt_trace_get_facility_by_num(tf->trace, 
                                          tf->event.facility_id);

  if(likely(tf->event.facility_id == LTT_FACILITY_CORE)) {
    switch((enum ltt_core_events)tf->event.event_id) {
  case LTT_EVENT_FACILITY_LOAD:
    size = strlen((char*)tf->event.data) + 1;
    g_debug("Update Event facility load of facility %s", (char*)tf->event.data);
    size += sizeof(struct LttFacilityLoad);
    break;
  case LTT_EVENT_FACILITY_UNLOAD:
    g_debug("Update Event facility unload");
    size = sizeof(struct LttFacilityUnload);
    break;
  case LTT_EVENT_STATE_DUMP_FACILITY_LOAD:
    size = strlen((char*)tf->event.data) + 1;
    g_debug("Update Event facility load state dump of facility %s",
        (char*)tf->event.data);
    size += sizeof(struct LttStateDumpFacilityLoad);
    break;
  case LTT_EVENT_HEARTBEAT:
    g_debug("Update Event heartbeat");
    size = sizeof(TimeHeartbeat);
    break;
  default:
    g_warning("Error in getting event size : tracefile %s, "
        "unknown event id %hhu in core facility.",
        g_quark_to_string(tf->name),
        tf->event.event_id);
    goto event_id_error;

    }
  } else {
    if(!f->exists) {
      g_error("Unknown facility %hhu (0x%hhx) in tracefile %s",
          tf->event.facility_id,
          tf->event.facility_id,
          g_quark_to_string(tf->name));
      goto facility_error;
    }

    LttEventType *event_type = 
      ltt_facility_eventtype_get(f, tf->event.event_id);

    if(!event_type) {
      g_error("Unknown event id %hhu in facility %s in tracefile %s",
          tf->event.event_id,
          g_quark_to_string(f->name),
          g_quark_to_string(tf->name));
      goto event_type_error;
    }
    
    if(event_type->root_field)
      size = get_field_type_size(tf, event_type,
          0, 0, event_type->root_field, tf->event.data);
    else
      size = 0;

    g_debug("Event root field : f.e %hhu.%hhu size %zd",
        tf->event.facility_id,
        tf->event.event_id, size);
  }
  
  tf->event.data_size = size;
  
  /* Check consistency between kernel and LTTV structure sizes */
  g_assert(tf->event.data_size == tf->event.event_size);
  
  return;

facility_error:
event_type_error:
event_id_error:
  tf->event.data_size = 0;
}


/* Take the tf current event offset and use the event facility id and event id
 * to figure out where is the next event offset.
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
  ssize_t event_size;
  
  /* seek over the buffer header if we are at the buffer start */
  if(tf->event.offset == 0) {
    tf->event.offset += sizeof(struct ltt_block_start_header);

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

found:
  return ret;

error:
  g_error("Error in ltt_seek_next_event for tracefile %s",
      g_quark_to_string(tf->name));
  return ENOPROTOOPT;
}


/*****************************************************************************
 *Function name
 *    calc_nsecs_per_cycle : calculate nsecs per cycle for current block
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

static double calc_nsecs_per_cycle(LttTracefile * tf)
{
  LttTime           lBufTotalTime; /* Total time for this buffer */
  double            lBufTotalNSec; /* Total time for this buffer in nsecs */
  LttCycleCount     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  lBufTotalTime = ltt_time_sub(tf->buffer.end.timestamp,
                               tf->buffer.begin.timestamp);

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle  = tf->buffer.end.cycle_count;
  lBufTotalCycle -= tf->buffer.begin.cycle_count;

  /* Convert the total time to double */
  lBufTotalNSec  = ltt_time_to_double(lBufTotalTime);
  
  return lBufTotalNSec / (double)lBufTotalCycle;

}
#if 0
void setFieldsOffset(LttTracefile *tf, LttEventType *evT,void *evD)
{
  LttField * rootFld = evT->root_field;
  //  rootFld->base_address = evD;

  if(likely(rootFld))
    rootFld->field_size = getFieldtypeSize(tf, evT->facility,
        evT, 0,0,rootFld, evD);  
}
#endif //0

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
        *fixed_root = final_child_status;
        *fixed_parent = final_child_status;
        field->field_size = 0;
        field->fixed_size = current_child_status;
      } else {
        field->field_size = max_size;
        field->fixed_size = FIELD_FIXED;
      }
      break;
  }

}


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
gint check_fields_compatibility(LttEventType *event_type1,
    LttEventType *event_type2,
    LttField *field1, LttField *field2)
{
  guint different = 0;
  enum field_status local_fixed_root, local_fixed_parent;
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

  size_t current_root_offset;
  size_t current_offset;
  enum field_status current_child_status, final_child_status;
  size_t max_size;

  if(type1->type_class != type2->type_class) {
    different = 1;
    goto end;
  }
  if(type1->element_name != type2->element_name) {
    different = 1;
    goto end;
  }
    
  switch(type1->type_class) {
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




#if 0
/*****************************************************************************
 *Function name
 *    getFieldtypeSize: get the size of the field type (primitive type)
 *Input params 
 *    evT             : event type
 *    offsetRoot      : offset from the root
 *    offsetParent    : offset from the parrent
 *    fld             : field
 *    evD             : event data, it may be NULL
 *Return value
 *    int             : size of the field
 ****************************************************************************/

static inline gint getFieldtypeSize(LttTracefile *tf,
       LttEventType * evT, gint offsetRoot,
	     gint offsetParent, LttField * fld, void *evD)
{
  gint size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  /* This likely has been tested with gcov : half of them.. */
  if(unlikely(fld->field_fixed == 1)){
    /* tested : none */
    if(unlikely(fld == evT->root_field)) {
      size = fld->field_size;
      goto end_getFieldtypeSize;
    }
  }

  /* From gcov profiling : half string, half struct, can we gain something
   * from that ? (Mathieu) */
  switch(type->type_class) {
    case LTT_ARRAY:
      element_number = (int) type->element_number;
      if(fld->field_fixed == -1){
        size = getFieldtypeSize(tf, evT, offsetRoot,
                                0,fld->child[0], NULL);
        if(size == 0){ //has string or sequence
          fld->field_fixed = 0;
        }else{
          fld->field_fixed = 1;
          size *= element_number; 
        }
      }else if(fld->field_fixed == 0){// has string or sequence
        size = 0;
        for(i=0;i<element_number;i++){
          size += getFieldtypeSize(tf, evT, offsetRoot+size,size, 
          fld->child[0], evD+size);
        }
      }else size = fld->field_size;
      if(unlikely(!evD)){
        fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
        fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
      }

      break;

    case LTT_SEQUENCE:
      size1 = (int) ltt_type_size(fac, type);
      if(fld->field_fixed == -1){
        fld->sequ_number_size = size1;
        fld->field_fixed = 0;
        size = getFieldtypeSize(evT, offsetRoot,
                                0,fld->child[0], NULL);      
        fld->element_size = size;
      }else{//0: sequence
        element_number = getIntNumber(tf,size1,evD);
        type->element_number = element_number;
        if(fld->element_size > 0){
          size = element_number * fld->element_size;
        }else{//sequence has string or sequence
          size = 0;
          for(i=0;i<element_number;i++){
            size += getFieldtypeSize(tf, evT,
                                     offsetRoot+size+size1,size+size1, 
                                     fld->child[0], evD+size+size1);
          }
        }
        size += size1;
      }
      if(unlikely(!evD)){
        fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
        fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
      }

      break;
      
    case LTT_STRING:
      size = 0;
      if(fld->field_fixed == -1){
        fld->field_fixed = 0;
      }else{//0: string
        /* Hope my implementation is faster than strlen (Mathieu) */
        char *ptr=(char*)evD;
        size = 1;
        /* from gcov : many many strings are empty, make it the common case.*/
        while(unlikely(*ptr != '\0')) { size++; ptr++; }
        //size = ptr - (char*)evD + 1; //include end : '\0'
      }
      fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
      fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;

      break;
      
    case LTT_STRUCT:
      element_number = (int) type->element_number;
      size = 0;
      /* tested with gcov */
      if(unlikely(fld->field_fixed == -1)){
        offset1 = offsetRoot;
        offset2 = 0;
        for(i=0;i<element_number;i++){
          size1=getFieldtypeSize(tf, evT,offset1,offset2,
                                 fld->child[i], NULL);
          if(likely(size1 > 0 && size >= 0)){
            size += size1;
            if(likely(offset1 >= 0)) offset1 += size1;
              offset2 += size1;
          }else{
            size = -1;
            offset1 = -1;
            offset2 = -1;
          }
        }
        if(unlikely(size == -1)){
           fld->field_fixed = 0;
           size = 0;
        }else fld->field_fixed = 1;
      }else if(likely(fld->field_fixed == 0)){
        offset1 = offsetRoot;
        offset2 = 0;
        for(i=0;unlikely(i<element_number);i++){
          size=getFieldtypeSize(tf, evT, offset1, offset2,
                                fld->child[i], evD+offset2);
          offset1 += size;
          offset2 += size;
        }      
        size = offset2;
      }else size = fld->field_size;
      fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
      fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
      break;

    default:
      if(unlikely(fld->field_fixed == -1)){
        size = (int) ltt_type_size(LTT_GET_BO(tf), type);
        fld->field_fixed = 1;
      }else size = fld->field_size;
      if(unlikely(!evD)){
        fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
        fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
      }
      break;
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  fld->field_size      = size;

end_getFieldtypeSize:

  return size;
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


/* get the start time of the trace */

LttTime ltt_trace_system_description_trace_start_time(LttSystemDescription *s)
{
  return s->trace_start;
}


LttTracefile *ltt_tracefile_new()
{
  return g_new(LttTracefile, 1);
}

void ltt_tracefile_destroy(LttTracefile *tf)
{
  g_free(tf);
}

void ltt_tracefile_copy(LttTracefile *dest, const LttTracefile *src)
{
  *dest = *src;
}

/* Before library loading... */

static void __attribute__((constructor)) init(void)
{
  LTT_FACILITY_NAME_HEARTBEAT = g_quark_from_string("heartbeat");
  LTT_EVENT_NAME_HEARTBEAT = g_quark_from_string("heartbeat");
  
  LTT_TRACEFILE_NAME_FACILITIES = g_quark_from_string("/control/facilities");
}

