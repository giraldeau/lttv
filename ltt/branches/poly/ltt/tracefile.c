/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang, Mathieu Desnoyers
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

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

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

#define DIR_NAME_SIZE 256
#define __UNUSED__ __attribute__((__unused__))

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)


/* obtain the time of an event */

static inline LttTime getEventTime(LttTracefile * tf);


/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void setFieldsOffset(LttTracefile *tf,LttEventType *evT,void *evD,LttTrace *t);

/* get the size of the field type according to the archtecture's
   size and endian type(info of the archecture) */
static inline gint getFieldtypeSize(LttTracefile * tf,
         LttEventType * evT, gint offsetRoot,
		     gint offsetParent, LttField *fld, void *evD, LttTrace* t);

/* read a fixed size or a block information from the file (fd) */
int readFile(int fd, void * buf, size_t size, char * mesg);
int readBlock(LttTracefile * tf, int whichBlock);

/* calculate cycles per nsec for current block */
void getCyclePerNsec(LttTracefile * t);

/* reinitialize the info of the block which is already in the buffer */
void updateTracefile(LttTracefile * tf);

/* go to the next event */
int skipEvent(LttTracefile * t);


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


/*****************************************************************************
 *Function name
 *    ltt_tracefile_open : open a trace file, construct a LttTracefile
 *Input params
 *    t                  : the trace containing the tracefile
 *    fileName           : path name of the trace file
 *Return value
 *                       : a pointer to a tracefile
 ****************************************************************************/ 

LttTracefile* ltt_tracefile_open(LttTrace * t, char * fileName)
{
  LttTracefile * tf;
  struct stat    lTDFStat;    /* Trace data file status */

  tf = g_new(LttTracefile, 1);  

  //open the file
  tf->name = g_strdup(fileName);
  tf->trace = t;
  tf->fd = open(fileName, O_RDONLY, 0);
  if(tf->fd < 0){
    g_warning("Unable to open input data file %s\n", fileName);
    g_free(tf->name);
    g_free(tf);
    return NULL;
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_warning("Unable to get the status of the input data file %s\n", fileName);
    g_free(tf->name);
    close(tf->fd);
    g_free(tf);
    return NULL;
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < (off_t)(sizeof(BlockStart) + EVENT_HEADER_SIZE)){
    g_print("The input data file %s does not contain a trace\n", fileName);
    g_free(tf->name);
    close(tf->fd);
    g_free(tf);
    return NULL;
  }
  
  //store the size of the file
  tf->file_size = lTDFStat.st_size;
  tf->block_size = t->system_description->ltt_block_size;
  tf->block_number = tf->file_size / tf->block_size;
  tf->which_block = 0;

  //allocate memory to contain the info of a block
  tf->buffer = (void *) g_new(char, t->system_description->ltt_block_size);

  //read the first block
  if(readBlock(tf,1)) exit(1);

  return tf;
}


/*****************************************************************************
 *Open control and per cpu tracefiles
 ****************************************************************************/

void ltt_tracefile_open_cpu(LttTrace *t, char * tracefile_name)
{
  LttTracefile * tf;
  tf = ltt_tracefile_open(t,tracefile_name);
  if(!tf) return;
  t->per_cpu_tracefile_number++;
  g_ptr_array_add(t->per_cpu_tracefiles, tf);
}

gint ltt_tracefile_open_control(LttTrace *t, char * control_name)
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
	fLoad.name = (char*)pos;
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

/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, 
 *Input params
 *    t                  : tracefile which will be closed
 ****************************************************************************/

void ltt_tracefile_close(LttTracefile *t)
{
  g_free(t->name);
  g_free(t->buffer);
  close(t->fd);
  g_free(t);
}


/*****************************************************************************
 *Get system information
 ****************************************************************************/
gint getSystemInfo(LttSystemDescription* des, char * pathname)
{
  FILE * fp;
  char buf[DIR_NAME_SIZE];

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

  fp = fopen(pathname,"r");
  if(!fp){
    g_warning("Can not open file : %s\n", pathname);
    return -1;
  }
  
  context = g_markup_parse_context_new(&markup_parser, 0, des,NULL);
  
  while(fgets(buf,DIR_NAME_SIZE, fp) != NULL){
    if(!g_markup_parse_context_parse(context, buf, DIR_NAME_SIZE, &error)){
      if(error != NULL) {
        g_warning("Can not parse xml file: \n%s\n", error->message);
        g_error_free(error);
      }
      g_markup_parse_context_free(context);
      fclose(fp);
      return -1;
    }
  }
  g_markup_parse_context_free(context);
  fclose(fp);
  return 0;
}

/*****************************************************************************
 *The following functions get facility/tracefile information
 ****************************************************************************/

gint getFacilityInfo(LttTrace *t, char* eventdefs)
{
  DIR * dir;
  struct dirent *entry;
  char * ptr;
  unsigned int i,j;
  LttFacility * f;
  LttEventType * et;
  char name[DIR_NAME_SIZE];

  dir = opendir(eventdefs);
  if(!dir) {
    g_warning("Can not open directory: %s\n", eventdefs);
    return -1;
  }

  while((entry = readdir(dir)) != NULL){
    ptr = &entry->d_name[strlen(entry->d_name)-4];
    if(strcmp(ptr,".xml") != 0) continue;
    strcpy(name,eventdefs);
    strcat(name,entry->d_name);
    ltt_facility_open(t,name);
  }  
  closedir(dir);
  
  for(j=0;j<t->facility_number;j++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, j);
    for(i=0; i<f->event_number; i++){
      et = f->events[i];
      setFieldsOffset(NULL, et, NULL, t);
    }    
  }
  return 0;
}

gint getControlFileInfo(LttTrace *t, char* control)
{
  DIR * dir;
  struct dirent *entry;
  char name[DIR_NAME_SIZE];

  dir = opendir(control);
  if(!dir) {
    g_warning("Can not open directory: %s\n", control);
    return -1;
  }

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,"facilities") != 0 &&
       strcmp(entry->d_name,"interrupts") != 0 &&
       strcmp(entry->d_name,"processes") != 0) continue;
    
    strcpy(name,control);
    strcat(name,entry->d_name);
    if(ltt_tracefile_open_control(t,name))
      return -1;
  }  
  closedir(dir);
  return 0;
}

gint getCpuFileInfo(LttTrace *t, char* cpu)
{
  DIR * dir;
  struct dirent *entry;
  char name[DIR_NAME_SIZE];

  dir = opendir(cpu);
  if(!dir) {
    g_warning("Can not open directory: %s\n", cpu);
    return -1;
  }

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,".") != 0 &&
       strcmp(entry->d_name,"..") != 0 &&
       strcmp(entry->d_name,".svn") != 0){      
      strcpy(name,cpu);
      strcat(name,entry->d_name);
      ltt_tracefile_open_cpu(t,name);
    }else continue;
  }  
  closedir(dir);
  return 0;
}

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
void get_absolute_pathname(const char *pathname, char * abs_pathname)
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

LttTrace *ltt_trace_open(const char *pathname)
{
  LttTrace  * t;
  LttSystemDescription * sys_description;
  char eventdefs[DIR_NAME_SIZE];
  char info[DIR_NAME_SIZE];
  char control[DIR_NAME_SIZE];
  char cpu[DIR_NAME_SIZE];
  char tmp[DIR_NAME_SIZE];
  char abs_path[DIR_NAME_SIZE];
  gboolean has_slash = FALSE;

  get_absolute_pathname(pathname, abs_path);
  //establish the pathname to different directories
  if(abs_path[strlen(abs_path)-1] == '/')has_slash = TRUE;
  strcpy(eventdefs,abs_path);
  if(!has_slash)strcat(eventdefs,"/");
  strcat(eventdefs,"eventdefs/");

  strcpy(info,abs_path);
  if(!has_slash)strcat(info,"/");
  strcat(info,"info/");

  strcpy(control,abs_path);
  if(!has_slash)strcat(control,"/");
  strcat(control,"control/");

  strcpy(cpu,abs_path);
  if(!has_slash)strcat(cpu,"/");
  strcat(cpu,"cpu/");

  //new trace
  sys_description = g_new(LttSystemDescription, 1);  
  t               = g_new(LttTrace, 1);
  t->pathname     = g_strdup(abs_path);
  t->facility_number          = 0;
  t->control_tracefile_number = 0;
  t->per_cpu_tracefile_number = 0;
  t->system_description = sys_description;
  t->control_tracefiles = g_ptr_array_new();
  t->per_cpu_tracefiles = g_ptr_array_new();
  t->facilities         = g_ptr_array_new();
  getDataEndianType(&(t->my_arch_size), &(t->my_arch_endian));

  //get system description  
  strcpy(tmp,info);
  strcat(tmp,"system.xml");
  if(getSystemInfo(sys_description, tmp)) {
    g_ptr_array_free(t->facilities, TRUE);
    g_ptr_array_free(t->per_cpu_tracefiles, TRUE);
    g_ptr_array_free(t->control_tracefiles, TRUE);
    g_free(sys_description);
    g_free(t->pathname);
    g_free(t);
    return NULL;
  }



  //get facilities info
  if(getFacilityInfo(t,eventdefs)) {
    g_ptr_array_free(t->facilities, TRUE);
    g_ptr_array_free(t->per_cpu_tracefiles, TRUE);
    g_ptr_array_free(t->control_tracefiles, TRUE);
    g_free(sys_description);
    g_free(t->pathname);
    g_free(t);
    return NULL;
  }
  
  //get control tracefile info
  getControlFileInfo(t,control);
  /*
  if(getControlFileInfo(t,control)) {
    g_ptr_array_free(t->facilities, TRUE);
    g_ptr_array_free(t->per_cpu_tracefiles, TRUE);
    g_ptr_array_free(t->control_tracefiles, TRUE);
    g_free(sys_description);
    g_free(t->pathname);
    g_free(t);
    return NULL;
  }*/ // With fatal error

  //get cpu tracefile info
  if(getCpuFileInfo(t,cpu)) {
    g_ptr_array_free(t->facilities, TRUE);
    g_ptr_array_free(t->per_cpu_tracefiles, TRUE);
    g_ptr_array_free(t->control_tracefiles, TRUE);
    g_free(sys_description);
    g_free(t->pathname);
    g_free(t);
    return NULL;
  }

  return t;
}

char * ltt_trace_name(LttTrace *t)
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
  return ltt_trace_open(self->pathname);
}

void ltt_trace_close(LttTrace *t)
{
  unsigned int i;
  LttTracefile * tf;
  LttFacility * f;

  g_free(t->pathname);
 
  //free system_description
  g_free(t->system_description->description);
  g_free(t->system_description->node_name);
  g_free(t->system_description->domain_name);
  g_free(t->system_description->kernel_name);
  g_free(t->system_description->kernel_release);
  g_free(t->system_description->kernel_version);
  g_free(t->system_description->machine);
  g_free(t->system_description->processor);
  g_free(t->system_description->hardware_platform);
  g_free(t->system_description->operating_system);
  g_free(t->system_description);

  //free control_tracefiles
  for(i=0;i<t->control_tracefile_number;i++){
    tf = (LttTracefile*)g_ptr_array_index(t->control_tracefiles,i);
    ltt_tracefile_close(tf);
  }
  g_ptr_array_free(t->control_tracefiles, TRUE);

  //free per_cpu_tracefiles
  for(i=0;i<t->per_cpu_tracefile_number;i++){
    tf = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles,i);
    ltt_tracefile_close(tf);
  }
  g_ptr_array_free(t->per_cpu_tracefiles, TRUE);

  //free facilities
  for(i=0;i<t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities,i);
    ltt_facility_close(f);
  }
  g_ptr_array_free(t->facilities, TRUE);

  g_free(t);
 
  g_blow_chunks();
}


/*****************************************************************************
 *Get the system description of the trace
 ****************************************************************************/

LttSystemDescription *ltt_trace_system_description(LttTrace *t)
{
  return t->system_description;
}

/*****************************************************************************
 * The following functions discover the facilities of the trace
 ****************************************************************************/

unsigned ltt_trace_facility_number(LttTrace *t)
{
  return (unsigned)(t->facility_number);
}

LttFacility *ltt_trace_facility_get(LttTrace *t, unsigned i)
{
  return (LttFacility*)g_ptr_array_index(t->facilities, i);
}

/*****************************************************************************
 *Function name
 *    ltt_trace_facility_find : find facilities in the trace
 *Input params
 *    t                       : the trace 
 *    name                    : facility name
 *Output params
 *    position                : position of the facility in the trace
 *Return value
 *                            : the number of facilities
 ****************************************************************************/

unsigned ltt_trace_facility_find(LttTrace *t, char *name, unsigned *position)
{
  unsigned int i, count=0;
  LttFacility * f;
  for(i=0;i<t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    if(strcmp(f->name,name)==0){
      count++;
      if(count==1) *position = i;      
    }else{
      if(count) break;
    }
  }
  return count;
}

/*****************************************************************************
 * Functions to discover all the event types in the trace 
 ****************************************************************************/

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

/* FIXME : performances could be improved with a better design for this
 * function : sequential search through a container has never been the
 * best on the critical path. */
LttFacility * ltt_trace_facility_by_id(LttTrace * trace, unsigned id)
{
  LttFacility * facility = NULL;
  unsigned int i;
  unsigned int num = trace->facility_number;
  GPtrArray *facilities = trace->facilities;

  for(i=0;unlikely(i<num);){
    LttFacility *iter_facility =
                      (LttFacility*) g_ptr_array_index(facilities,i);
    unsigned base_id = iter_facility->base_id;

    if(likely(id >= base_id && 
       id < base_id + iter_facility->event_number)) {
      facility = iter_facility;
      break;
    } else {
      i++;
    }
  }
  
  return facility;
}

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned evId)
{
  LttEventType *event_type;
  
  LttFacility * f;
  f = ltt_trace_facility_by_id(t,evId);

  if(unlikely(!f)) event_type = NULL;
  else event_type = f->events[evId - f->base_id];

  return event_type;
}

/*****************************************************************************
 *There is one "per cpu" tracefile for each CPU, numbered from 0 to
 *the maximum number of CPU in the system. When the number of CPU installed
 *is less than the maximum, some positions are unused. There are also a
 *number of "control" tracefiles (facilities, interrupts...). 
 ****************************************************************************/
unsigned ltt_trace_control_tracefile_number(LttTrace *t)
{
  return t->control_tracefile_number;
}

unsigned ltt_trace_per_cpu_tracefile_number(LttTrace *t)
{
  return t->per_cpu_tracefile_number;
}

/*****************************************************************************
 *It is possible to search for the tracefiles by name or by CPU position.
 *The index within the tracefiles of the same type is returned if found
 *and a negative value otherwise. 
 ****************************************************************************/

int ltt_trace_control_tracefile_find(LttTrace *t, const gchar *name)
{
  LttTracefile * tracefile;
  unsigned int i;
  for(i=0;i<t->control_tracefile_number;i++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->control_tracefiles, i);
    if(strcmp(tracefile->name, name)==0)break;
  }
  if(i == t->control_tracefile_number) return -1;
  return i;
}

/* not really useful. We just have to know that cpu tracefiles
 * comes before control tracefiles.
 */
int ltt_trace_per_cpu_tracefile_find(LttTrace *t, const gchar *name)
{
  LttTracefile * tracefile;
  unsigned int i;
  for(i=0;i<t->per_cpu_tracefile_number;i++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, i);
    if(strcmp(tracefile->name, name)==0)break;
  }
  if(i == t->per_cpu_tracefile_number) return -1;
  return i;
}

/*****************************************************************************
 *Get a specific tracefile 
 ****************************************************************************/

LttTracefile *ltt_trace_control_tracefile_get(LttTrace *t, unsigned i)
{
  return (LttTracefile*)g_ptr_array_index(t->control_tracefiles, i);  
}

LttTracefile *ltt_trace_per_cpu_tracefile_get(LttTrace *t, unsigned i)
{
  return (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, i);
}

/*****************************************************************************
 * Get the start time and end time of the trace 
 ****************************************************************************/

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end)
{
  LttTime startSmall, startTmp, endBig, endTmp;
  unsigned int i, j=0;
  LttTracefile * tf;

  for(i=0;i<t->control_tracefile_number;i++){
    tf = g_ptr_array_index(t->control_tracefiles, i);
    readBlock(tf,1);
    startTmp = tf->a_block_start->time;    
    readBlock(tf,tf->block_number);
    endTmp = tf->a_block_end->time;
    if(i==0){
      startSmall = startTmp;
      endBig     = endTmp;
      j = 1;
      continue;
    }
    if(ltt_time_compare(startSmall,startTmp) > 0) startSmall = startTmp;
    if(ltt_time_compare(endBig,endTmp) < 0) endBig = endTmp;
  }

  for(i=0;i<t->per_cpu_tracefile_number;i++){
    tf = g_ptr_array_index(t->per_cpu_tracefiles, i);
    readBlock(tf,1);
    startTmp = tf->a_block_start->time;    
    readBlock(tf,tf->block_number);
    endTmp = tf->a_block_end->time;
    if(j == 0 && i==0){
      startSmall = startTmp;
      endBig     = endTmp;
      continue;
    }
    if(ltt_time_compare(startSmall,startTmp) > 0) startSmall = startTmp;
    if(ltt_time_compare(endBig,endTmp) < 0) endBig = endTmp;
  }

  if(start != NULL) *start = startSmall;
  if(end != NULL) *end = endBig;
}


/*****************************************************************************
 *Get the name of a tracefile
 ****************************************************************************/

char *ltt_tracefile_name(LttTracefile *tf)
{
  return tf->name;
}

/*****************************************************************************
 * Get the number of blocks in the tracefile 
 ****************************************************************************/

unsigned ltt_tracefile_block_number(LttTracefile *tf)
{
  return tf->block_number; 
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_seek_time: seek to the first event of the trace with time 
 *                             larger or equal to time
 *Input params
 *    t                      : tracefile
 *    time                   : criteria of the time
 ****************************************************************************/
void ltt_tracefile_find_time_block(LttTracefile *t, LttTime time, 
				   int start_block, int end_block)
{
  int err, tmp_block, s, e; 
  int headTime;
  int tailTime;
  
  err=readBlock(t,start_block);
  if(err) g_error("Can not read tracefile: %s\n", t->name); 
  if(start_block == end_block)return;

  tailTime = ltt_time_compare(t->a_block_end->time, time);
  if(tailTime >= 0) return;
  
  err=readBlock(t,end_block);
  if(err) g_error("Can not read tracefile: %s\n", t->name); 
  if(start_block+1 == end_block)return;
  
  headTime = ltt_time_compare(t->a_block_start->time, time);
  if(headTime <= 0 ) return;
  
  tmp_block = (end_block + start_block)/2;
  err=readBlock(t,tmp_block);
  if(err) g_error("Can not read tracefile: %s\n", t->name); 

  headTime = ltt_time_compare(t->a_block_start->time, time);
  tailTime = ltt_time_compare(t->a_block_end->time, time);
  if(headTime <= 0 && tailTime >= 0) return;
  
  if(headTime > 0){
    s = start_block + 1;
    e = tmp_block - 1;
    if(s <= e)
      ltt_tracefile_find_time_block(t, time, s, e);
    else return;
  }

  if(tailTime < 0){
    s = tmp_block + 1;
    e = end_block - 1;
    if(s <= e)
      ltt_tracefile_find_time_block(t, time, s, e);
    else return;
  }  
}

void ltt_tracefile_backward_find_time_block(LttTracefile *t, LttTime time)
{
  int t_time, h_time, err;
  err=readBlock(t,t->which_block-1);
  if(err) g_error("Can not read tracefile: %s\n", t->name); 
  h_time = ltt_time_compare(t->a_block_start->time, time);
  t_time = ltt_time_compare(t->a_block_end->time, time);
  if(h_time == 0){
    int tmp;
    if(t->which_block == 1) return;
    err=readBlock(t,t->which_block-1);
    if(err) g_error("Can not read tracefile: %s\n", t->name); 
    tmp = ltt_time_compare(t->a_block_end->time, time);
    if(tmp == 0) return ltt_tracefile_seek_time(t, time);
    err=readBlock(t,t->which_block+1);
    if(err) g_error("Can not read tracefile: %s\n", t->name);     
  }else if(h_time > 0){
    ltt_tracefile_find_time_block(t, time, 1, t->which_block);
    return ltt_tracefile_seek_time(t, time) ;    
  }else{
    if(t_time >= 0) return ltt_tracefile_seek_time(t, time);
    err=readBlock(t,t->which_block+1);
    if(err) g_error("Can not read tracefile: %s\n", t->name);    
  }
}

void ltt_tracefile_seek_time(LttTracefile *t, LttTime time)
{
  int err;
  LttTime lttTime;
  int headTime = ltt_time_compare(t->a_block_start->time, time);
  int tailTime = ltt_time_compare(t->a_block_end->time, time);
  LttEvent ev;

  if(headTime < 0 && tailTime > 0){
    if(ltt_time_compare(t->a_block_end->time, t->current_event_time) !=0) {
      lttTime = getEventTime(t);
      err = ltt_time_compare(lttTime, time);
      if(err > 0){
	if(t->which_event==2 || ltt_time_compare(t->prev_event_time,time)<0){
	  return;
	}else{
	  updateTracefile(t);
	  return ltt_tracefile_seek_time(t, time);
	}
      }else if(err < 0){
	while(1){
	  if(ltt_tracefile_read(t,&ev) == NULL) {
	    g_print("End of file\n");      
	    return;
	  }
	  lttTime = getEventTime(t);
	  err = ltt_time_compare(lttTime, time);
	  if(err >= 0)return;
	}
      }else return;    
    }else{//we are at the end of the block
      updateTracefile(t);
      return ltt_tracefile_seek_time(t, time);      
    }
  }else if(headTime >= 0){
    if(t->which_block == 1){
      updateTracefile(t);      
    }else{
      if(ltt_time_compare(t->prev_block_end_time, time) >= 0 ||
	 (t->prev_block_end_time.tv_sec == 0 && 
	  t->prev_block_end_time.tv_nsec == 0 )){
	ltt_tracefile_backward_find_time_block(t, time);
      }else{
	updateTracefile(t);
      }
    }
  }else if(tailTime < 0){
    if(t->which_block != t->block_number){
      ltt_tracefile_find_time_block(t, time, t->which_block+1, t->block_number);
      return ltt_tracefile_seek_time(t, time);
    }else {
     t->cur_event_pos = t->buffer + t->block_size;
     g_print("End of file\n");      
      return;      
    }    
  }else if(tailTime == 0){
    t->cur_event_pos = t->last_event_pos;
    t->current_event_time = time;  
    t->cur_heart_beat_number = 0;
    t->prev_event_time.tv_sec = 0;
    t->prev_event_time.tv_nsec = 0;
    return;
  }
}

/*****************************************************************************
 * Seek to the first event with position equal or larger to ep 
 *
 * Modified by Mathieu Desnoyers to used faster offset position instead of
 * re-reading the whole buffer.
 ****************************************************************************/

void ltt_tracefile_seek_position(LttTracefile *t, const LttEventPosition *ep)
{
  //if we are at the right place, just return
  if(likely(t->which_block == ep->block_num && t->which_event == ep->event_num))
    return;
  
  if(likely(t->which_block == ep->block_num)) updateTracefile(t);
  else readBlock(t,ep->block_num);
  //event offset is available
  if(likely(ep->old_position)){
    int err;

    t->which_event = ep->event_num;
    t->cur_event_pos = t->buffer + ep->event_offset;
    t->prev_event_time = ep->event_time;
    t->current_event_time = ep->event_time;
    t->cur_heart_beat_number = ep->heart_beat_number;
    t->cur_cycle_count = ep->event_cycle_count;

    /* This is a workaround for fast position seek */
    t->last_event_pos = ep->last_event_pos;
    t->prev_block_end_time = ep->prev_block_end_time;
    t->prev_event_time = ep->prev_event_time;
    t->pre_cycle_count = ep->pre_cycle_count;
    t->count = ep->count;
    t->overflow_nsec = ep->overflow_nsec;
    t->last_heartbeat = ep->last_heartbeat;
    /* end of workaround */

    //update the fields of the current event and go to the next event
    err = skipEvent(t);
    if(unlikely(err == ERANGE)) g_error("event id is out of range\n");
      
    return;
  }

  //only block number and event index are available
  //MD: warning : this is slow!
  g_warning("using slow O(n) tracefile seek position");

  LttEvent event;
  while(likely(t->which_event < ep->event_num)) ltt_tracefile_read(t, &event);

  return;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_read : read the current event, set the pointer to the next
 *Input params
 *    t                  : tracefile
 *Return value
 *    LttEvent *        : an event to be processed
 ****************************************************************************/

LttEvent *ltt_tracefile_read(LttTracefile *t, LttEvent *event)
{
  int err;

  if(unlikely(t->cur_event_pos == t->buffer + t->block_size)){
    if(unlikely(t->which_block == t->block_number)){
      return NULL;
    }
    err = readBlock(t, t->which_block + 1);
    if(unlikely(err))g_error("Can not read tracefile");    
  }

  event->event_id = (int)(*(guint16 *)(t->cur_event_pos));
  if(unlikely(event->event_id == TRACE_TIME_HEARTBEAT))
    t->cur_heart_beat_number++;

  t->prev_event_time  = t->current_event_time;
  //  t->current_event_time = getEventTime(t);

  event->time_delta = *(guint32 *)(t->cur_event_pos + EVENT_ID_SIZE);
  event->event_time = t->current_event_time;
  event->event_cycle_count = t->cur_cycle_count;

  event->tracefile = t;
  event->data = t->cur_event_pos + EVENT_HEADER_SIZE;  
  event->which_block = t->which_block;
  event->which_event = t->which_event;

  /* This is a workaround for fast position seek */
  event->last_event_pos = t->last_event_pos;
  event->prev_block_end_time = t->prev_block_end_time;
  event->prev_event_time = t->prev_event_time;
  event->pre_cycle_count = t->pre_cycle_count;
  event->count = t->count;
  event->overflow_nsec = t->overflow_nsec;
  event->last_heartbeat = t->last_heartbeat;
  
  /* end of workaround */



  //update the fields of the current event and go to the next event
  err = skipEvent(t);
  if(unlikely(err == ERANGE)) g_error("event id is out of range\n");

  return event;
}

/****************************************************************************
 *Function name
 *    readFile    : wrap function to read from a file
 *Input Params
 *    fd          : file descriptor
 *    buf         : buf to contain the content
 *    size        : number of bytes to be read
 *    mesg        : message to be printed if some thing goes wrong
 *return value 
 *    0           : success
 *    EIO         : can not read from the file
 ****************************************************************************/

int readFile(int fd, void * buf, size_t size, char * mesg)
{
   ssize_t nbBytes = read(fd, buf, size);

   if((size_t)nbBytes != size) {
     if(nbBytes < 0) {
       perror("Error in readFile : ");
     } else {
       g_warning("%s",mesg);
     }
     return EIO;
   }
   return 0;
}


/****************************************************************************
 *Function name
 *    readBlock       : read a block from the file
 *Input Params
 *    lttdes          : ltt trace file 
 *    whichBlock      : the block which will be read
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 ****************************************************************************/

int readBlock(LttTracefile * tf, int whichBlock)
{
  off_t nbBytes;
  guint32 lostSize;

  /* same block already opened requested */
  if((guint)whichBlock == tf->which_block) return 0;
  
  if(likely(whichBlock - tf->which_block == 1 && tf->which_block != 0)){
    tf->prev_block_end_time = tf->a_block_end->time;
    tf->prev_event_time     = tf->a_block_end->time;
  }else{
    tf->prev_block_end_time.tv_sec = 0;
    tf->prev_block_end_time.tv_nsec = 0;
    tf->prev_event_time.tv_sec = 0;
    tf->prev_event_time.tv_nsec = 0;
  }

  nbBytes=lseek(tf->fd,(off_t)((whichBlock-1)*tf->block_size), SEEK_SET);
  if(unlikely(nbBytes == -1)) return EINVAL;
  
  if(unlikely(readFile(tf->fd,tf->buffer,tf->block_size,"Unable to read a block")))
    return EIO;

  tf->a_block_start=(BlockStart *) (tf->buffer + EVENT_HEADER_SIZE);
  lostSize = *(guint32 *)(tf->buffer + tf->block_size - sizeof(guint32));
  tf->a_block_end=(BlockEnd *)(tf->buffer + tf->block_size
	           			- sizeof(guint32) - lostSize - sizeof(BlockEnd)); 
  tf->last_event_pos = tf->buffer + tf->block_size - 
                              sizeof(guint32) - lostSize
                              - sizeof(BlockEnd) - EVENT_HEADER_SIZE;

  tf->which_block = whichBlock;
  tf->which_event = 1;
  tf->cur_event_pos = tf->buffer;//the beginning of the block, block start ev
  tf->cur_heart_beat_number = 0;
  tf->last_heartbeat = NULL;
  
  /* read the whole block to precalculate total of cycles in it */
  tf->count = 0;
  tf->pre_cycle_count = 0;
  tf->cur_cycle_count = 0;

  getCyclePerNsec(tf);

  tf->overflow_nsec = (-((double)(tf->a_block_start->cycle_count&0xFFFFFFFF))
                                        * tf->nsec_per_cycle);

  tf->current_event_time = getEventTime(tf);  

  return 0;  
}

/*****************************************************************************
 *Function name
 *    updateTracefile : reinitialize the info of the block which is already 
 *                      in the buffer
 *Input params 
 *    tf              : tracefile
 ****************************************************************************/

void updateTracefile(LttTracefile * tf)
{
  tf->which_event = 1;
  tf->cur_event_pos = tf->buffer;
  tf->current_event_time = getEventTime(tf);  
  tf->cur_heart_beat_number = 0;

  tf->prev_event_time.tv_sec = 0;
  tf->prev_event_time.tv_nsec = 0;
  tf->count = 0;

  tf->overflow_nsec = (-((double)tf->a_block_start->cycle_count)
                                        * tf->nsec_per_cycle);

}

/*****************************************************************************
 *Function name
 *    skipEvent : go to the next event, update the fields of the current event
 *Input params 
 *    t         : tracefile
 *return value 
 *    0               : success
 *    ERANGE          : event id is out of range
 ****************************************************************************/

int skipEvent(LttTracefile * t)
{
  int evId;
  void * evData;
  LttEventType * evT;
  LttField * rootFld;

  evId   = (int)(*(guint16 *)(t->cur_event_pos));
  evData = t->cur_event_pos + EVENT_HEADER_SIZE;

  evT    = ltt_trace_eventtype_get(t->trace,(unsigned)evId);
    
  if(likely(evT)) rootFld = evT->root_field;
  else return ERANGE;
  
  if(likely(rootFld)){
    //event has string/sequence or the last event is not the same event
    if(likely((evT->latest_block!=t->which_block || evT->latest_event!=t->which_event)
       && rootFld->field_fixed == 0)){
      setFieldsOffset(t, evT, evData, t->trace);
    }
    t->cur_event_pos += EVENT_HEADER_SIZE + rootFld->field_size;
  }else t->cur_event_pos += EVENT_HEADER_SIZE;
    
  evT->latest_block = t->which_block;
  evT->latest_event = t->which_event;

  //the next event is in the next block
  if(unlikely(evId == TRACE_BLOCK_END)){
    t->cur_event_pos = t->buffer + t->block_size;
  }else{
    t->which_event++;
    t->current_event_time = getEventTime(t);
  }

  return 0;
}


/*****************************************************************************
 *Function name
 *    getCyclePerNsec : calculate cycles per nsec for current block
 *    MD: should have tracefile_read the whole block, so we know the
 *    total of cycles in it before being called.
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

void getCyclePerNsec(LttTracefile * t)
{
  LttTime           lBufTotalTime; /* Total time for this buffer */
  double            lBufTotalNSec; /* Total time for this buffer in nsecs */
  LttCycleCount     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  lBufTotalTime = ltt_time_sub(t->a_block_end->time, t->a_block_start->time);

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle  = t->a_block_end->cycle_count;
  lBufTotalCycle -= t->a_block_start->cycle_count;

  /* Convert the total time to double */
  lBufTotalNSec  = ltt_time_to_double(lBufTotalTime);
  
  t->nsec_per_cycle = (double)lBufTotalNSec / (double)lBufTotalCycle;

  /* Pre-multiply one overflow (2^32 cycles) by nsec_per_cycle */
  t->one_overflow_nsec = t->nsec_per_cycle * (double)0x100000000ULL;

}

/****************************************************************************
 *Function name
 *    getEventTime    : obtain the time of an event 
 *                      NOTE : this function _really_ is on critical path.
 *Input params 
 *    tf              : tracefile
 *Return value
 *    LttTime        : the time of the event
 ****************************************************************************/

static inline LttTime getEventTime(LttTracefile * tf)
{
  LttTime       time;
  LttCycleCount cycle_count;      // cycle count for the current event
  //LttCycleCount lEventTotalCycle; // Total cycles from start for event
  gint64       lEventNSec;       // Total nsecs from start for event
  LttTime       lTimeOffset;      // Time offset in struct LttTime
  guint16       evId;

  evId = *(guint16 *)tf->cur_event_pos;
  
  cycle_count = (LttCycleCount)*(guint32 *)(tf->cur_event_pos + EVENT_ID_SIZE);

  gboolean comp_count = cycle_count < tf->pre_cycle_count;

  tf->pre_cycle_count = cycle_count;
  
  if(unlikely(comp_count)) {
    /* Overflow */
    tf->overflow_nsec += tf->one_overflow_nsec;
    tf->count++; //increment overflow count
  }

  if(unlikely(evId == TRACE_BLOCK_START)) {
     lEventNSec = 0;
  } else if(unlikely(evId == TRACE_BLOCK_END)) {
    lEventNSec = ((double)
                 (tf->a_block_end->cycle_count - tf->a_block_start->cycle_count)
                           * tf->nsec_per_cycle);
  }
#if 0
  /* If you want to make heart beat a special case and use their own 64 bits
   * TSC, activate this.
   */
  else if(unlikely(evId == TRACE_TIME_HEARTBEAT)) {

    tf->last_heartbeat = (TimeHeartbeat*)(tf->cur_event_pos+EVENT_HEADER_SIZE);
    lEventNSec = ((double)(tf->last_heartbeat->cycle_count 
                           - tf->a_block_start->cycle_count)
                  * tf->nsec_per_cycle);
  }
#endif //0
  else {
    lEventNSec = (gint64)((double)cycle_count * tf->nsec_per_cycle)
                                +tf->overflow_nsec;
  }

  lTimeOffset = ltt_time_from_uint64(lEventNSec);
  
  time = ltt_time_add(tf->a_block_start->time, lTimeOffset);  
  
  return time;
}

/*****************************************************************************
 *Function name
 *    setFieldsOffset : set offset of the fields
 *Input params 
 *    tracefile       : opened trace file  
 *    evT             : the event type
 *    evD             : event data, it may be NULL
 ****************************************************************************/

void setFieldsOffset(LttTracefile *tf,LttEventType *evT,void *evD,LttTrace* t)
{
  LttField * rootFld = evT->root_field;
  //  rootFld->base_address = evD;

  if(likely(rootFld))
    rootFld->field_size = getFieldtypeSize(tf, evT, 0,0,rootFld, evD,t);  
}

/*****************************************************************************
 *Function name
 *    getFieldtypeSize: get the size of the field type (primitive type)
 *Input params 
 *    tracefile       : opened trace file 
 *    evT             : event type
 *    offsetRoot      : offset from the root
 *    offsetParent    : offset from the parrent
 *    fld             : field
 *    evD             : event data, it may be NULL
 *Return value
 *    int             : size of the field
 ****************************************************************************/

static inline gint getFieldtypeSize(LttTracefile * t,
       LttEventType * evT, gint offsetRoot,
	     gint offsetParent, LttField * fld, void *evD, LttTrace *trace)
{
  gint size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  if(unlikely(t && evT->latest_block==t->which_block &&
                   evT->latest_event==t->which_event)){
    size = fld->field_size;
    goto end_getFieldtypeSize;
  } else {
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
          size = getFieldtypeSize(t, evT, offsetRoot,
                                  0,fld->child[0], NULL, trace);
          if(size == 0){ //has string or sequence
            fld->field_fixed = 0;
          }else{
            fld->field_fixed = 1;
            size *= element_number; 
          }
        }else if(fld->field_fixed == 0){// has string or sequence
          size = 0;
          for(i=0;i<element_number;i++){
            size += getFieldtypeSize(t, evT, offsetRoot+size,size, 
            fld->child[0], evD+size, trace);
          }
        }else size = fld->field_size;
        if(unlikely(!evD)){
          fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
          fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        }

        break;

      case LTT_SEQUENCE:
        size1 = (int) ltt_type_size(trace, type);
        if(fld->field_fixed == -1){
          fld->sequ_number_size = size1;
          fld->field_fixed = 0;
          size = getFieldtypeSize(t, evT, offsetRoot,
                                  0,fld->child[0], NULL, trace);      
          fld->element_size = size;
        }else{//0: sequence
          element_number = getIntNumber(size1,evD);
          type->element_number = element_number;
          if(fld->element_size > 0){
            size = element_number * fld->element_size;
          }else{//sequence has string or sequence
            size = 0;
            for(i=0;i<element_number;i++){
              size += getFieldtypeSize(t, evT, offsetRoot+size+size1,size+size1, 
                                       fld->child[0], evD+size+size1, trace);
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
            size1=getFieldtypeSize(t, evT,offset1,offset2,
                                   fld->child[i], NULL, trace);
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
            size=getFieldtypeSize(t,evT,offset1,offset2,
                                  fld->child[i],evD+offset2, trace);
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
          size = (int) ltt_type_size(trace, type);
          fld->field_fixed = 1;
        }else size = fld->field_size;
        if(unlikely(!evD)){
          fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
          fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        }
        break;
    }
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  fld->field_size      = size;

end_getFieldtypeSize:

  return size;
}


/*****************************************************************************
 *Function name
 *    getIntNumber    : get an integer number
 *Input params 
 *    size            : the size of the integer
 *    evD             : the event data
 *Return value
 *    gint64          : a 64 bits integer
 ****************************************************************************/

gint64 getIntNumber(int size, void *evD)
{
  gint64 i;

  switch(size) {
    case 1: i = *(gint8 *)evD; break;
    case 2: i = *(gint16 *)evD; break;
    case 4: i = *(gint32 *)evD; break;
    case 8: i = *(gint64 *)evD; break;
    default: i = *(gint64 *)evD;
             g_critical("getIntNumber : integer size %d unknown", size);
             break;
  }

  return (gint64)i;
}

/*****************************************************************************
 *Function name
 *    getDataEndianType : get the data type size and endian type of the local
 *                        machine
 *Input params 
 *    size              : size of data type
 *    endian            : endian type, little or big
 ****************************************************************************/

void getDataEndianType(LttArchSize * size, LttArchEndian * endian)
{
  int i = 1;
  char c = (char) i;
  int sizeInt=sizeof(int), sizeLong=sizeof(long), sizePointer=sizeof(void *);

  if(c == 1) *endian = LTT_LITTLE_ENDIAN;
  else *endian = LTT_BIG_ENDIAN;

  if(sizeInt == 2 && sizeLong == 4 && sizePointer == 4) 
    *size = LTT_LP32;
  else if(sizeInt == 4 && sizeLong == 4 && sizePointer == 4) 
    *size = LTT_ILP32;
  else if(sizeInt == 4 && sizeLong == 8 && sizePointer == 8) 
    *size = LTT_LP64;
  else if(sizeInt == 8 && sizeLong == 8 && sizePointer == 8) 
    *size = LTT_ILP64;
  else *size = LTT_UNKNOWN;
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

