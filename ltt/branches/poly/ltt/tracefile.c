#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/errno.h>  

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/trace.h>
#include <ltt/facility.h>

#define DIR_NAME_SIZE 256

/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void setFieldsOffset(LttTracefile *tf,LttEventType *evT,void *evD,LttTrace *t);

/* get the size of the field type according to the archtecture's
   size and endian type(info of the archecture) */
int getFieldtypeSize(LttTracefile * tf, LttEventType * evT, int offsetRoot,
		     int offsetParent, LttField *fld, void *evD, LttTrace* t);

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
static void parser_start_element (GMarkupParseContext  *context,
				  const gchar          *element_name,
				  const gchar         **attribute_names,
				  const gchar         **attribute_values,
				  gpointer              user_data,
				  GError              **error)
{
  int i=0;
  LttSystemDescription* des = (LttSystemDescription* )user_data;
  if(strcmp("system", element_name)){
    g_warning("This is not system.xml file\n");
    exit(1);
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
      g_warning("Not a valid attribute\n");
      exit(1);      
    }
    i++;
  }
}

static void parser_end_element   (GMarkupParseContext  *context,
				  const gchar          *element_name,
				  gpointer              user_data,
				  GError              **error)
{
}

static void  parser_characters   (GMarkupParseContext  *context,
				  const gchar          *text,
				  gsize                 text_len,
				  gpointer              user_data,
				  GError              **error)
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
  BlockStart     a_block_start;

  tf = g_new(LttTracefile, 1);  

  //open the file
  tf->name = g_strdup(fileName);
  tf->trace = t;
  tf->fd = open(fileName, O_RDONLY, 0);
  if(tf->fd < 0){
    g_error("Unable to open input data file %s\n", fileName);
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_error("Unable to get the status of the input data file %s\n", fileName);
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < sizeof(BlockStart) + EVENT_HEADER_SIZE){
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

void ltt_tracefile_open_control(LttTrace *t, char * control_name)
{
  LttTracefile * tf;
  LttEvent * ev;
  LttFacility * f;
  guint16 evId;
  void * pos;
  FacilityLoad fLoad;
  int i;

  tf = ltt_tracefile_open(t,control_name);
  if(!tf) return;
  t->control_tracefile_number++;
  g_ptr_array_add(t->control_tracefiles,tf);

  //parse facilities tracefile to get base_id
  if(strcmp(&control_name[strlen(control_name)-10],"facilities") ==0){
    while(1){
      ev = ltt_tracefile_read(tf);
      if(!ev)return; // end of file

      if(ev->event_id == TRACE_FACILITY_LOAD){
	pos = ev->data;
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
	if(i==t->facility_number)
	  g_error("Facility: %s, checksum: %d is not founded\n",
		  fLoad.name,fLoad.checksum);
      }else if(ev->event_id == TRACE_BLOCK_START){
	continue;
      }else if(ev->event_id == TRACE_BLOCK_END){
	break;
      }else g_error("Not valid facilities trace file\n");
    }
  }
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
void getSystemInfo(LttSystemDescription* des, char * pathname)
{
  FILE * fp;
  char buf[DIR_NAME_SIZE];
  char description[4*DIR_NAME_SIZE];

  GMarkupParseContext * context;
  GError * error;
  GMarkupParser markup_parser =
    {
      parser_start_element,
      parser_end_element,
      parser_characters,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  fp = fopen(pathname,"r");
  if(!fp){
    g_error("Can not open file : %s\n", pathname);
  }
  
  context = g_markup_parse_context_new(&markup_parser, 0, des,NULL);
  
  while(fgets(buf,DIR_NAME_SIZE, fp) != NULL){
    if(!g_markup_parse_context_parse(context, buf, DIR_NAME_SIZE, &error)){
      g_warning("Can not parse xml file: \n%s\n", error->message);
      exit(1);
    }
  }
  fclose(fp);
}

/*****************************************************************************
 *The following functions get facility/tracefile information
 ****************************************************************************/

void getFacilityInfo(LttTrace *t, char* eventdefs)
{
  DIR * dir;
  struct dirent *entry;
  char * ptr;
  int i,j;
  LttFacility * f;
  LttEventType * et;
  char name[DIR_NAME_SIZE];

  dir = opendir(eventdefs);
  if(!dir) g_error("Can not open directory: %s\n", eventdefs);

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
}

void getControlFileInfo(LttTrace *t, char* control)
{
  DIR * dir;
  struct dirent *entry;
  char name[DIR_NAME_SIZE];

  dir = opendir(control);
  if(!dir) g_error("Can not open directory: %s\n", control);

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,"facilities") != 0 &&
       strcmp(entry->d_name,"interrupts") != 0 &&
       strcmp(entry->d_name,"processes") != 0) continue;
    
    strcpy(name,control);
    strcat(name,entry->d_name);
    ltt_tracefile_open_control(t,name);
  }  
  closedir(dir);
}

void getCpuFileInfo(LttTrace *t, char* cpu)
{
  DIR * dir;
  struct dirent *entry;
  char name[DIR_NAME_SIZE];

  dir = opendir(cpu);
  if(!dir) g_error("Can not open directory: %s\n", cpu);

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,".") != 0 &&
       strcmp(entry->d_name,"..") != 0 ){
      strcpy(name,cpu);
      strcat(name,entry->d_name);
      ltt_tracefile_open_cpu(t,name);
    }else continue;
  }  
  closedir(dir);
}

/*****************************************************************************
 *A trace is specified as a pathname to the directory containing all the
 *associated data (control tracefiles, per cpu tracefiles, event 
 *descriptions...).
 *
 *When a trace is closed, all the associated facilities, types and fields
 *are released as well.
 *
 * MD : If pathname is already absolute, we do not add current working
 * directory to it.
 *
 ****************************************************************************/

void get_absolute_pathname(const char *pathname, char * abs_pathname)
{
  char * ptr, *ptr1;
  size_t size = DIR_NAME_SIZE;
  abs_pathname[0] = '\0';

	if(pathname[0] == '/')
	{
    strcat(abs_pathname, pathname);
		return;
	}

  if(!getcwd(abs_pathname, size)){
    g_warning("Can not get current working directory\n");
    strcat(abs_pathname, pathname);
    return;
  }

  strcat(abs_pathname,"/");
  
  ptr = (char*)pathname;
  ptr1 = ptr + 1;
  while(*ptr == '.' && *ptr1 == '.'){
    ptr += 3;
    ptr1 = ptr + 1;
  }
  strcat(abs_pathname,ptr);
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
  t               = g_new(LttTrace, 1);
  sys_description = g_new(LttSystemDescription, 1);  
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
  getSystemInfo(sys_description, tmp);

  //get facilities info
  getFacilityInfo(t,eventdefs);
  
  //get control tracefile info
  getControlFileInfo(t,control);

  //get cpu tracefile info
  getCpuFileInfo(t,cpu);

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
  int i;
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
  int i, count=0;
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
  int i;
  unsigned count = 0;
  LttFacility * f;
  for(i=0;i<t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    count += f->event_number;
  }
  return count;
}

LttFacility * ltt_trace_facility_by_id(LttTrace * trace, unsigned id)
{
  LttFacility * facility;
  int i;
  for(i=0;i<trace->facility_number;i++){
    facility = (LttFacility*) g_ptr_array_index(trace->facilities,i);
    if(id >= facility->base_id && 
       id < facility->base_id + facility->event_number)
      break;
  }
  if(i==trace->facility_number) return NULL;
  else return facility;
}

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned evId)
{
  LttFacility * f;
  f = ltt_trace_facility_by_id(t,evId);
  if(!f) return NULL;
  return f->events[evId - f->base_id];
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

int ltt_trace_control_tracefile_find(LttTrace *t, char *name)
{
  LttTracefile * tracefile;
  int i;
  for(i=0;i<t->control_tracefile_number;i++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->control_tracefiles, i);
    if(strcmp(tracefile->name, name)==0)break;
  }
  if(i == t->control_tracefile_number) return -1;
  return i;
}

int ltt_trace_per_cpu_tracefile_find(LttTrace *t, unsigned i)
{
  LttTracefile * tracefile;
  int j, name;
  for(j=0;j<t->per_cpu_tracefile_number;j++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, j);
    name = atoi(tracefile->name);
    if(name == (int)i)break;
  }
  if(j == t->per_cpu_tracefile_number) return -1;
  return j;
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
  int i, j=0;
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

  *start = startSmall;
  *end = endBig;
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
  LttEvent * ev;

  if(headTime < 0 && tailTime > 0){
    if(ltt_time_compare(t->a_block_end->time, t->current_event_time) !=0) {
      lttTime = getEventTime(t);
      err = ltt_time_compare(lttTime, time);
      if(err > 0){
	if(t->which_event==2 || (&t->prev_event_time,&time)<0){
	  return;
	}else{
	  updateTracefile(t);
	  return ltt_tracefile_seek_time(t, time);
	}
      }else if(err < 0){
	while(1){
	  ev = ltt_tracefile_read(t);
	  if(ev == NULL){
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
 ****************************************************************************/

void ltt_tracefile_seek_position(LttTracefile *t, LttEventPosition *ep)
{
  //if we are at the right place, just return
  if(t->which_block == ep->block_num && t->which_event == ep->event_num)
    return;
  
  if(t->which_block == ep->block_num) updateTracefile(t);
  else readBlock(t,ep->block_num);

  //event offset is availiable
  if(ep->old_position){
    t->cur_heart_beat_number = ep->heart_beat_number;
    t->cur_event_pos = t->buffer + ep->event_offset;
    return;
  }

  //only block number and event index are availiable
  while(t->which_event < ep->event_num) ltt_tracefile_read(t);

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

LttEvent *ltt_tracefile_read(LttTracefile *t)
{
  LttEvent * lttEvent = &t->an_event;
  int err;

  if(t->cur_event_pos == t->buffer + t->block_size){
    if(t->which_block == t->block_number){
      return NULL;
    }
    err = readBlock(t, t->which_block + 1);
    if(err)g_error("Can not read tracefile");    
  }

  lttEvent->event_id = (int)(*(guint16 *)(t->cur_event_pos));
  if(lttEvent->event_id == TRACE_TIME_HEARTBEAT)
    t->cur_heart_beat_number++;

  t->prev_event_time  = t->current_event_time;
  //  t->current_event_time = getEventTime(t);

  lttEvent->time_delta = *(guint32 *)(t->cur_event_pos + EVENT_ID_SIZE);
  lttEvent->event_time = t->current_event_time;

  lttEvent->tracefile = t;
  lttEvent->data = t->cur_event_pos + EVENT_HEADER_SIZE;  
  lttEvent->which_block = t->which_block;
  lttEvent->which_event = t->which_event;

  //update the fields of the current event and go to the next event
  err = skipEvent(t);
  if(err == ERANGE) g_error("event id is out of range\n");

  lttEvent->event_cycle_count = t->cur_cycle_count;

  return lttEvent;
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
   ssize_t nbBytes;
   nbBytes = read(fd, buf, size);
   if(nbBytes != size){
     printf("%s\n",mesg);
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

  if(whichBlock - tf->which_block == 1 && tf->which_block != 0){
    tf->prev_block_end_time = tf->a_block_end->time;
    tf->prev_event_time     = tf->a_block_end->time;
  }else{
    tf->prev_block_end_time.tv_sec = 0;
    tf->prev_block_end_time.tv_nsec = 0;
    tf->prev_event_time.tv_sec = 0;
    tf->prev_event_time.tv_nsec = 0;
  }

  nbBytes=lseek(tf->fd,(off_t)((whichBlock-1)*tf->block_size), SEEK_SET);
  if(nbBytes == -1) return EINVAL;
  
  if(readFile(tf->fd,tf->buffer,tf->block_size,"Unable to read a block")) 
    return EIO;

  tf->a_block_start=(BlockStart *) (tf->buffer + EVENT_HEADER_SIZE);
  lostSize = *(guint32 *)(tf->buffer + tf->block_size - sizeof(guint32));
  tf->a_block_end=(BlockEnd *)(tf->buffer + tf->block_size - 
				lostSize + EVENT_HEADER_SIZE); 
  tf->last_event_pos = tf->buffer + tf->block_size - lostSize;

  tf->which_block = whichBlock;
  tf->which_event = 1;
  tf->cur_event_pos = tf->buffer;//the beginning of the block, block start ev
  tf->cur_heart_beat_number = 0;

  getCyclePerNsec(tf);

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
  int evId, err;
  void * evData;
  LttEventType * evT;
  LttField * rootFld;

  evId   = (int)(*(guint16 *)(t->cur_event_pos));
  evData = t->cur_event_pos + EVENT_HEADER_SIZE;

  evT    = ltt_trace_eventtype_get(t->trace,(unsigned)evId);
    
  if(evT) rootFld = evT->root_field;
  else return ERANGE;
  
  if(rootFld){
    //event has string/sequence or the last event is not the same event
    if((evT->latest_block!=t->which_block || evT->latest_event!=t->which_event) 
       && rootFld->field_fixed == 0){
      setFieldsOffset(t, evT, evData, t->trace);
    }
    t->cur_event_pos += EVENT_HEADER_SIZE + rootFld->field_size;
  }else t->cur_event_pos += EVENT_HEADER_SIZE;
    
  evT->latest_block = t->which_block;
  evT->latest_event = t->which_event;

  //the next event is in the next block
  if(evId == TRACE_BLOCK_END){
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
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

void getCyclePerNsec(LttTracefile * t)
{
  LttTime           lBufTotalTime; /* Total time for this buffer */
  LttCycleCount     lBufTotalNSec; /* Total time for this buffer in nsecs */
  LttCycleCount     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  lBufTotalTime = ltt_time_sub(t->a_block_end->time, t->a_block_start->time);

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle  = t->a_block_end->cycle_count;
  lBufTotalCycle -= t->a_block_start->cycle_count;

  /* Convert the total time to nsecs */
  lBufTotalNSec  = lBufTotalTime.tv_sec;
  lBufTotalNSec *= NANOSECONDS_PER_SECOND; 
  lBufTotalNSec += lBufTotalTime.tv_nsec;
  
  t->cycle_per_nsec = (double)lBufTotalCycle / (double)lBufTotalNSec;
}

/****************************************************************************
 *Function name
 *    getEventTime    : obtain the time of an event 
 *Input params 
 *    tf              : tracefile
 *Return value
 *    LttTime        : the time of the event
 ****************************************************************************/

LttTime getEventTime(LttTracefile * tf)
{
  LttTime       time;
  LttCycleCount cycle_count;      // cycle count for the current event
  LttCycleCount lEventTotalCycle; // Total cycles from start for event
  double        lEventNSec;       // Total usecs from start for event
  LttTime       lTimeOffset;      // Time offset in struct LttTime
  guint16       evId;
  gint64        nanoSec, tmpCycleCount = (((guint64)1)<<32);

  evId = *(guint16 *)tf->cur_event_pos;
  if(evId == TRACE_BLOCK_START){
    tf->count = 0;
    tf->pre_cycle_count = 0;
    tf->cur_cycle_count = tf->a_block_start->cycle_count;
    return tf->a_block_start->time;
  }else if(evId == TRACE_BLOCK_END){
    tf->count = 0;
    tf->pre_cycle_count = 0;
    tf->cur_cycle_count = tf->a_block_end->cycle_count;
    return tf->a_block_end->time;
  }
  
  // Calculate total time in cycles from start of buffer for this event
  cycle_count = (LttCycleCount)*(guint32 *)(tf->cur_event_pos + EVENT_ID_SIZE);
  
  if(cycle_count < tf->pre_cycle_count)tf->count++;
  tf->pre_cycle_count = cycle_count;
  cycle_count += tmpCycleCount * tf->count;  
  
  if(tf->cur_heart_beat_number > tf->count)
    cycle_count += tmpCycleCount * (tf->cur_heart_beat_number - tf->count);  

  tf->cur_cycle_count = cycle_count;

  lEventTotalCycle  = cycle_count;
  lEventTotalCycle -= tf->a_block_start->cycle_count;

  // Convert it to nsecs
  lEventNSec = lEventTotalCycle / tf->cycle_per_nsec;
  nanoSec    = lEventNSec;

  // Determine offset in struct LttTime 
  lTimeOffset.tv_nsec = nanoSec % NANOSECONDS_PER_SECOND;
  lTimeOffset.tv_sec  = nanoSec / NANOSECONDS_PER_SECOND;

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

  if(rootFld)
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

int getFieldtypeSize(LttTracefile * t, LttEventType * evT, int offsetRoot,
	     int offsetParent, LttField * fld, void *evD, LttTrace *trace)
{
  int size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  if(t){
    if(evT->latest_block==t->which_block && evT->latest_event==t->which_event){
      return fld->field_size;
    } 
  }

  if(fld->field_fixed == 1){
    if(fld == evT->root_field) return fld->field_size;
  }     

  if(type->type_class != LTT_STRUCT && type->type_class != LTT_ARRAY &&
     type->type_class != LTT_SEQUENCE && type->type_class != LTT_STRING){
    if(fld->field_fixed == -1){
      size = (int) ltt_type_size(trace, type);
      fld->field_fixed = 1;
    }else size = fld->field_size;

  }else if(type->type_class == LTT_ARRAY){
    element_number = (int) type->element_number;
    if(fld->field_fixed == -1){
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL, trace);
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

  }else if(type->type_class == LTT_SEQUENCE){
    size1 = (int) ltt_type_size(trace, type);
    if(fld->field_fixed == -1){
      fld->sequ_number_size = size1;
      fld->field_fixed = 0;
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL, trace);      
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

  }else if(type->type_class == LTT_STRING){
    size = 0;
    if(fld->field_fixed == -1){
      fld->field_fixed = 0;
    }else{//0: string
      size = strlen((char*)evD) + 1; //include end : '\0'
    }

  }else if(type->type_class == LTT_STRUCT){
    element_number = (int) type->element_number;
    size = 0;
    if(fld->field_fixed == -1){      
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size1=getFieldtypeSize(t, evT,offset1,offset2, fld->child[i], NULL, trace);
	if(size1 > 0 && size >= 0){
	  size += size1;
	  if(offset1 >= 0) offset1 += size1;
	  offset2 += size1;
	}else{
	  size = -1;
	  offset1 = -1;
	  offset2 = -1;
	}
      }
      if(size == -1){
	fld->field_fixed = 0;
	size = 0;
      }else fld->field_fixed = 1;
    }else if(fld->field_fixed == 0){
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size=getFieldtypeSize(t,evT,offset1,offset2,fld->child[i],evD+offset2, trace);
	offset1 += size;
	offset2 += size;
      }      
      size = offset2;
    }else size = fld->field_size;
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  if(!evD){
    fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
    fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
  }
  fld->field_size      = size;

  return size;
}


/*****************************************************************************
 *Function name
 *    getIntNumber    : get an integer number
 *Input params 
 *    size            : the size of the integer
 *    evD             : the event data
 *Return value
 *    int             : an integer
 ****************************************************************************/

int getIntNumber(int size, void *evD)
{
  gint64 i;
  if(size == 1)      i = *(gint8 *)evD;
  else if(size == 2) i = *(gint16 *)evD;
  else if(size == 4) i = *(gint32 *)evD;
  else if(size == 8) i = *(gint64 *)evD;
 
  return (int) i;
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

