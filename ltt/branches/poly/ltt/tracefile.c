#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/errno.h>  

#include "LTTTypes.h"  
#include "parser.h"
#include <ltt/tracefile.h>

#include "default.h"  //yxx test

/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void setFieldsOffset(ltt_tracefile * t, ltt_eventtype *evT, void *evD);

/* get the size of the field type according to the archtecture's
   size and endian type(info of the archecture) */
int getFieldtypeSize(ltt_tracefile * t, ltt_eventtype * evT, int offsetRoot,
		     int offsetParent, ltt_field * fld, void * evD );

/* read a fixed size or a block information from the file (fd) */
int readFile(int fd, void * buf, size_t size, char * mesg);
int readBlock(ltt_tracefile * tf, int whichBlock);

/* calculate cycles per nsec for current block */
void getCyclePerNsec(ltt_tracefile * t);

/* functions to release the memory occupied by the tables of a tracefile */
void freeKey (gpointer key, gpointer value, gpointer user_data);
void freeValue (gpointer key, gpointer value, gpointer user_data);
void freeFacilityNameTable (gpointer key, gpointer value, gpointer user_data);

/* reinitialize the info of the block which is already in the buffer */
void updateTracefile(ltt_tracefile * tf);

/* go to the next event */
int skipEvent(ltt_tracefile * t);

/* compare two time (ltt_time), 0:t1=t2, -1:t1<t2, 1:t1>t2 */
int timecmp(ltt_time * t1, ltt_time * t2);

/* get an integer number */
int getIntNumber(int size1, void *evD);


/* Time operation macros for ltt_time (struct timespec) */
/*  (T3 = T2 - T1) */
#define TimeSub(T3, T2, T1) \
do \
{\
  (T3).tv_sec  = (T2).tv_sec  - (T1).tv_sec;  \
  (T3).tv_nsec = (T2).tv_nsec - (T1).tv_nsec; \
  if((T3).tv_nsec < 0)\
    {\
    (T3).tv_sec--;\
    (T3).tv_nsec += 1000000000;\
    }\
} while(0)

/*  (T3 = T2 + T1) */
#define TimeAdd(T3, T2, T1) \
do \
{\
  (T3).tv_sec  = (T2).tv_sec  + (T1).tv_sec;  \
  (T3).tv_nsec = (T2).tv_nsec + (T1).tv_nsec; \
  if((T3).tv_nsec >= 1000000000)\
    {\
    (T3).tv_sec += (T3).tv_nsec / 1000000000;\
    (T3).tv_nsec = (T3).tv_nsec % 1000000000;\
    }\
} while(0)



/*****************************************************************************
 *Function name
 *    ltt_tracefile_open : open a trace file, construct a ltt_tracefile
 *Input params
 *    pathname           : path name of the trace file
 *Return value
 *    ltt_tracefile *    : the structure represents the opened trace file
 ****************************************************************************/ 

ltt_tracefile *ltt_tracefile_open(char * fileName)
{
  ltt_tracefile * tf;
  struct stat      lTDFStat;    /* Trace data file status */
  trace_header_event *trace_header;
  block_header   a_block_header;

  tf = g_new(ltt_tracefile, 1);

  //open the file
  tf->fd = open(fileName, O_RDONLY, 0);
  if(tf->fd < 0){
    g_error("Unable to open input data file %s\n", fileName);
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_error("Unable to get the status of the input data file %s\n", fileName);
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < sizeof(block_header) + sizeof(trace_header_event)){
    g_error("The input data file %s does not contain a trace\n", fileName);
  }
  
  //store the size of the file
  tf->file_size = lTDFStat.st_size;

  //read the first block header
  if(readFile(tf->fd,(void*)&a_block_header, sizeof(block_header),
              "Unable to read block header"))
    exit(1);
  
  //read the trace header event
  trace_header = g_new(trace_header_event, 1);
  if(readFile(tf->fd,(void*)trace_header, sizeof(trace_header_event),
              "Unable to read trace header event"))
    exit(1);

  tf->block_number = tf->file_size / trace_header->buffer_size;
  tf->trace_header = (trace_header_event *)trace_header;

  //allocate memory to contain the info of a block
  tf->buffer = (void *) g_new(char, trace_header->buffer_size);

  //read the last block, get the trace end time
  if(readBlock(tf,tf->block_number)) exit(1);
  tf->end_time = tf->a_block_footer->time;

  //read the first block, get the trace start time
  if(tf->block_number != 1)
    if(readBlock(tf,1)) exit(1);
  tf->start_time = tf->a_block_header->time;

  //get local machine's data type size and endian type
  getDataEndianType(&(tf->my_arch_size), &(tf->my_arch_endian));

  //initialize all tables
  tf->eventtype_number   = 0;
  tf->facility_number    = 0;
  tf->index_facility     = g_hash_table_new (g_int_hash, g_int_equal);
  tf->facility_name      = g_hash_table_new (g_str_hash, g_str_equal);
  tf->base_id_name       = g_hash_table_new (g_str_hash, g_int_equal);
  tf->eventtype_event_id = g_ptr_array_new();

  return tf;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, release all facilities if their
 *                         usage count == 0 when close_facilities is true
 *Input params
 *    t                  : tracefile which will be closed
 *    close_facilities   : bool to show if the facilities need to be released
 *Return value
 *    int                : ????
 ****************************************************************************/

int ltt_tracefile_close(ltt_tracefile *t, int close_facilities)
{
  //free index_facility table
  g_hash_table_foreach (t->index_facility, freeKey, NULL);
  g_hash_table_destroy(t->index_facility);

  //free base_id_facility table
  g_hash_table_foreach (t->base_id_name, freeValue, NULL);
  g_hash_table_destroy(t->base_id_name);

  //free eventtype_event_id array
  g_ptr_array_free (t->eventtype_event_id, TRUE);

  //free facility_name table
  g_hash_table_foreach (t->facility_name, freeFacilityNameTable,
			GINT_TO_POINTER(1));
  g_hash_table_destroy(t->facility_name);  

  //free tracefile structure
  g_free(t->trace_header);
  g_free(t->buffer);
  g_free(t);
  return 0;
}

/*****************************************************************************
 * functions to release the memory occupied by the tables of a tracefile
 ****************************************************************************/

void freeKey (gpointer key, gpointer value, gpointer user_data)
{
  g_free(key);
}

void freeValue (gpointer key, gpointer value, gpointer user_data)
{
  g_free(value);
}

void freeFacilityNameTable (gpointer key, gpointer value, gpointer user_data)
{
  ltt_facility * fac = (ltt_facility*) value;
  fac->usage_count--;
  if(GPOINTER_TO_INT(user_data) != 0)
    ltt_facility_close(fac);    
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_facility_add: increases the facility usage count and also 
 *                                specifies the base of the numeric range
 *                                assigned to the event types in the facility 
 *                                for this tracefile
 *Input params
 *    t                  : tracefile that the facility will be added
 *    f                  : facility that will be attached to the tracefile
 *    base_id            : the id for the first event of the facility in the 
 *                         trace file
 *Return value
 *    int                : ????
 ****************************************************************************/

int ltt_tracefile_facility_add(ltt_tracefile *t,ltt_facility *f,int base_id)
{
  int * id, *index;
  int i, j, k;
  ltt_eventtype * et;
  gpointer tmpPtr;

  //increase the facility usage count
  f->usage_count++;
  t->eventtype_number += f->event_number;
  t->facility_number++;

  //insert the facility into index_facility table
  id = g_new(int,1);
  *id = t->facility_number;
  g_hash_table_insert(t->index_facility, (gpointer)id,(gpointer)f);  

  //insert facility name into table: base_id_name
  id = g_new(int,1);
  *id = base_id;
  g_hash_table_insert(t->base_id_name, (gpointer)(f->name), (gpointer)id);

  //insert facility name into table: facility_name
  g_hash_table_insert(t->facility_name, (gpointer)(f->name), (gpointer)f);
  
  //insert eventtype into the array: eventtype_event_id
  j = base_id + f->event_number;
  k = t->eventtype_event_id->len;
  if(j > t->eventtype_event_id->len){
    for(i=0; i < j - k; i++){
      tmpPtr = (gpointer)g_new(ptr_wrap, 1);
      g_ptr_array_add(t->eventtype_event_id,tmpPtr);
    }
  }

  //initialize the unused elements: NULL
  if(j-k > f->event_number){
    for(i=k; i<base_id; i++){
      tmpPtr = g_ptr_array_index(t->eventtype_event_id, i);
      ((ptr_wrap*)tmpPtr)->ptr = NULL;
    }
  }

  //initialize the elements with the eventtypes belonging to the facility
  for(i=0; i<f->event_number; i++){
    tmpPtr = g_ptr_array_index(t->eventtype_event_id, base_id + i);
    ((ptr_wrap*)tmpPtr)->ptr = (gpointer)(f->events[i]);
  }
  
  //update offset
  for(i=0; i<f->event_number; i++){
    et = f->events[i];
    setFieldsOffset(t, et, NULL);
  }


  return 0;
}

/*****************************************************************************
 * The following functions used to query the info of the machine where the 
 * tracefile was generated.  A tracefile may be queried for its architecture 
 * type(e.g.,"i386", "powerpc", "powerpcle", "s390", "s390x"), its architecture
 * variant(e.g., "att" versus "sun" for m68k), its operating system (e.g., 
 * "linux","bsd"), its generic architecture, and the machine identity (e.g., 
 * system host name). All character strings belong to the associated tracefile 
 * and are freed when it is closed
 ****************************************************************************/

uint32_t ltt_tracefile_arch_type(ltt_tracefile *t)
{
  return t->trace_header->arch_type;
}  

uint32_t ltt_tracefile_arch_variant(ltt_tracefile *t)
{
  return t->trace_header->arch_variant;
}

ltt_arch_size ltt_tracefile_arch_size(ltt_tracefile *t)
{
  return t->trace_header->arch_size;
}

ltt_arch_endian ltt_tracefile_arch_endian(ltt_tracefile *t)
{
  return t->trace_header->arch_endian;
} 

uint32_t ltt_tracefile_system_type(ltt_tracefile *t)
{
  return t->trace_header->system_type;
}

char *ltt_tracefile_system_name(ltt_tracefile *t)
{
  return t->trace_header->system_name;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_cpu_number: get the number of the cpu
 *Input params
 *    t                       : tracefile
 *Return value
 *    unsigned                : the number of cpu the system has
 ****************************************************************************/

unsigned ltt_tracefile_cpu_number(ltt_tracefile *t)
{
  return (unsigned)(t->trace_header->cpu_number);
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_cpu_single: does the tracefile contain events only for a 
 *                              single CPU ?
 *Input params
 *    t                       : tracefile
 *Return value
 *    int                     : 1 for YES, 0 for NO
 ****************************************************************************/

int ltt_tracefile_cpu_single(ltt_tracefile *t)
{
  if(t->trace_header->cpu_number_used == 1) return 1;
  else return 0;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_cpu_id: which CPU is contained in the tracefile?
 *Input params
 *    t                   : tracefile
 *Return value
 *    unsigned            : cpu id
 ****************************************************************************/

unsigned ltt_tracefile_cpu_id(ltt_tracefile *t)
{
  return (unsigned)(t->trace_header->cpu_id);
}

/*****************************************************************************
 * The following functions get the times related to the tracefile
 ****************************************************************************/

ltt_time ltt_tracefile_time_start(ltt_tracefile *t)
{
  return t->start_time;
}

ltt_time ltt_tracefile_time_end(ltt_tracefile *t)
{
  return t->end_time;
}

ltt_time ltt_tracefile_duration(ltt_tracefile *t)
{
  ltt_time T;
  TimeSub(T, t->end_time, t->start_time);
  return T;
}

/*****************************************************************************
 * The following functions discover the facilities added to the tracefile
 ****************************************************************************/

unsigned ltt_tracefile_facility_number(ltt_tracefile *t)
{
  return (unsigned)(t->facility_number);
}

ltt_facility *ltt_tracefile_facility_get(ltt_tracefile *t, unsigned i)
{
  gconstpointer ptr = (gconstpointer)(&i);
  if(i<=0 || i> t->facility_number)return NULL;
  return (ltt_facility*)g_hash_table_lookup(t->index_facility,ptr);
}

ltt_facility *ltt_tracefile_facility_get_by_name(ltt_tracefile *t,char *name)
{
  ltt_facility * fac;
  fac=(ltt_facility*)g_hash_table_lookup(t->facility_name,(gconstpointer)name);
  return fac;
}

/*****************************************************************************
 * The following functions to discover all the event types in the facilities 
 * added to the tracefile. The event type integer id, unique for the trace, 
 * is used
 ****************************************************************************/

unsigned ltt_tracefile_eventtype_number(ltt_tracefile *t)
{
  return t->eventtype_number;
}

ltt_eventtype *ltt_tracefile_eventtype_get(ltt_tracefile *t, unsigned evId)
{
  ptr_wrap * ptr;
  ptr = (ptr_wrap *)g_ptr_array_index(t->eventtype_event_id, (gint)evId);
  return (ltt_eventtype *)(ptr->ptr);
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_eventtype_id
 *                        : given an event type, find its unique id within 
 *                          the tracefile
 *Input params
 *    t                   : tracefile
 *    et                  : event type
 *Return value
 *    unsigned            : id of the event type in the tracefile
 ****************************************************************************/

unsigned ltt_tracefile_eventtype_id(ltt_tracefile *t, ltt_eventtype *et)
{
  int *id;
  char *name = et->facility->name;
  
  id = (int*)g_hash_table_lookup(t->base_id_name, (gconstpointer)name);
  if(!id)return 0;
  return (unsigned)(*id + et->index);
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_eventtype_root_field
 *                        : get the root field associated with an event type
 *                          for the tracefile
 *Input params
 *    t                   : tracefile
 *    id                  : event id
 *Return value
 *    ltt_field *         : root field of the event
 ****************************************************************************/

ltt_field *ltt_tracefile_eventtype_root_field(ltt_tracefile *t, unsigned id)
{
  ltt_eventtype * et;
  et = ltt_tracefile_eventtype_get(t, id);
  if(!et) return NULL;
  return et->root_field;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_seek_time: seek to the first event of the trace with time 
 *                             larger or equal to time
 *Input params
 *    t                      : tracefile
 *    time                   : criteria of the time
 *Return value
 *    int                    : error code
 *                             ENOENT, end of the file
 *                             EINVAL, lseek fail
 *                             EIO, can not read from the file
 ****************************************************************************/

int ltt_tracefile_seek_time(ltt_tracefile *t, ltt_time time)
{
  int err;
  ltt_time lttTime;
  int headTime = timecmp(&(t->a_block_header->time), &time);
  int tailTime = timecmp(&(t->a_block_footer->time), &time);
  
  if(headTime < 0 && tailTime > 0){
    lttTime = getEventTime(t);
    err = timecmp(&lttTime, &time);
    if(err >= 0){
      if( ( (t->which_block != 1 && t->which_event != 0) ||
            (t->which_block == 1 && t->which_event != 1)   ) &&
          ((t->prev_event_time.tv_sec==0 && t->prev_event_time.tv_nsec==0) ||
	   timecmp(&t->prev_event_time, &time) >= 0 )  ){
	updateTracefile(t);
	return ltt_tracefile_seek_time(t, time);
      }
    }else if(err < 0){
      err = t->which_block;
      if(ltt_tracefile_read(t) == NULL) return ENOENT;      
      if(t->which_block == err)
	return ltt_tracefile_seek_time(t,time);
    }    
  }else if(headTime > 0){
    if(t->which_block == 1){
      updateTracefile(t);      
    }else{
      if( (t->prev_block_end_time.tv_sec == 0 && 
	   t->prev_block_end_time.tv_nsec == 0  ) ||
	   timecmp(&(t->prev_block_end_time),&time) > 0 ){
	err=readBlock(t,t->which_block-1);
	if(err) return err; 
	return ltt_tracefile_seek_time(t, time) ;
      }else{
	updateTracefile(t);
      }
    }
  }else if(tailTime <= 0){
    if(t->which_block != t->block_number){
      err=readBlock(t,t->which_block+1);
      if(err) return err; 
    }else return ENOENT;    
    if(tailTime < 0) return ltt_tracefile_seek_time(t, time);
  }else if(headTime == 0){
    updateTracefile(t);
  }
  return 0;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_read : read the next event 
 *Input params
 *    t                  : tracefile
 *Return value
 *    ltt_event *        : an event to be processed
 ****************************************************************************/

ltt_event *ltt_tracefile_read(ltt_tracefile *t)
{
  ltt_event * lttEvent = (ltt_event *)g_new(ltt_event, 1);
  ltt_eventtype * evT;
  ltt_facility * fac;

  //update the fields of the current event and go to the next event
  if(skipEvent(t)) return NULL;

  t->current_event_time = getEventTime(t);

  lttEvent->event_id = (int)(*(uint8_t *)(t->cur_event_pos));
  evT = ltt_tracefile_eventtype_get(t, (unsigned)lttEvent->event_id);
  fac = evT->facility;
  if(evT->index == TRACE_EV_HEARTBEAT && strcmp(fac->name, "default")==0)
    t->cur_heart_beat_number++;
  lttEvent->cycle_count=*(uint32_t*)(t->cur_event_pos + EVENT_ID_SIZE);
  lttEvent->tracefile = t;
  lttEvent->data = t->cur_event_pos + EVENT_HEADER_SIZE;  

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

int readBlock(ltt_tracefile * tf, int whichBlock)
{
  off_t nbBytes;
  uint32_t lostSize;

  if(whichBlock - tf->which_block == 1 && tf->which_block != 0){
    tf->prev_block_end_time = tf->a_block_footer->time;
  }else{
    tf->prev_block_end_time.tv_sec = 0;
    tf->prev_block_end_time.tv_nsec = 0;
  }
  tf->prev_event_time.tv_sec = 0;
  tf->prev_event_time.tv_nsec = 0;

  nbBytes=lseek(tf->fd,(off_t)((whichBlock-1)*tf->trace_header->buffer_size),
                  SEEK_SET);
  if(nbBytes == -1) return EINVAL;
  
  if(readFile(tf->fd,tf->buffer,tf->trace_header->buffer_size,
              "Unable to read a block")) return EIO;


  tf->a_block_header=(block_header *) tf->buffer;
  lostSize = *(uint32_t*)(tf->buffer + tf->trace_header->buffer_size 
			  - sizeof(uint32_t));
  /* skip event ID and time delta to get the address of the block foot */
  tf->a_block_footer=(block_footer *)(tf->buffer+tf->trace_header->buffer_size
                                   -lostSize+sizeof(uint8_t)+sizeof(uint32_t)); 
  tf->which_block = whichBlock;
  tf->which_event = 0;
  tf->first_event_pos = tf->buffer + sizeof(block_header);
  if(tf->which_block == 1){
    tf->which_event++;
    tf->first_event_pos += sizeof(trace_header_event);
  }
  tf->cur_event_pos = tf->first_event_pos;
  tf->current_event_time = tf->a_block_header->time;
  tf->cur_heart_beat_number = 0;
  
  getCyclePerNsec(tf);

  return 0;  
}

/*****************************************************************************
 *Function name
 *    updateTracefile : reinitialize the info of the block which is already 
 *                      in the buffer
 *Input params 
 *    tf              : tracefile
 ****************************************************************************/

void updateTracefile(ltt_tracefile * tf)
{
  if(tf->which_block == 1)tf->which_event = 1;
  else tf->which_event = 0;
  tf->cur_event_pos = tf->first_event_pos;
  tf->current_event_time = tf->a_block_header->time;  
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
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 *    ENOENT          : end of file
 *    ERANGE          : event id is out of range
 ****************************************************************************/

int skipEvent(ltt_tracefile * t)
{
  int evId, err;
  void * evData;
  ltt_eventtype * evT;
  ltt_field * rootFld;

  evId   = (int)(*(uint8_t *)(t->cur_event_pos));
  evData = t->cur_event_pos + EVENT_HEADER_SIZE;
  evT    = ltt_tracefile_eventtype_get(t,(unsigned)evId);

  if(evT) rootFld = evT->root_field;
  else return ERANGE;

  t->prev_event_time = getEventTime(t);
  
  //event has string/sequence or the last event is not the same event
  if((evT->latest_block!=t->which_block || evT->latest_event!=t->which_event) 
     && rootFld->field_fixed == 0){
    setFieldsOffset(t, evT, evData);
  }
  t->cur_event_pos += EVENT_HEADER_SIZE + rootFld->field_size;

  evT->latest_block = t->which_block;
  evT->latest_event = t->which_event;
  
  //the next event is in the next block
  if(t->which_event == t->a_block_header->event_count - 1){
    if(t->which_block == t->block_number) return ENOENT;
    err = readBlock(t, t->which_block + 1);
    if(err) return err;
  }else{
    t->which_event++;
  }

  return 0;
}

/*****************************************************************************
 *Function name
 *    getCyclePerNsec : calculate cycles per nsec for current block
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

void getCyclePerNsec(ltt_tracefile * t)
{
  ltt_time            lBufTotalTime; /* Total time for this buffer */
  ltt_cycle_count     lBufTotalNSec; /* Total time for this buffer in nsecs */
  ltt_cycle_count     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  TimeSub(lBufTotalTime,t->a_block_footer->time, t->a_block_header->time);

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle = t->a_block_footer->cycle_count 
                   - t->a_block_header->cycle_count;

  /* Convert the total time to nsecs */
  lBufTotalNSec = lBufTotalTime.tv_sec * 1000000000 + lBufTotalTime.tv_nsec;
  
  t->cycle_per_nsec = (double)lBufTotalCycle / (double)lBufTotalNSec;
}

/****************************************************************************
 *Function name
 *    getEventTime    : obtain the time of an event 
 *Input params 
 *    tf              : tracefile
 *Return value
 *    ltt_time        : the time of the event
 ****************************************************************************/

ltt_time getEventTime(ltt_tracefile * tf)
{
  ltt_time        time;
  ltt_cycle_count cycle_count;      /* cycle count for the current event */
  ltt_cycle_count lEventTotalCycle; /* Total cycles from start for event */
  double          lEventNSec;       /* Total usecs from start for event */
  ltt_time        lTimeOffset;      /* Time offset in struct ltt_time */
  
  /* Calculate total time in cycles from start of buffer for this event */
  cycle_count = *(uint32_t*)(tf->cur_event_pos + EVENT_ID_SIZE);
  if(tf->cur_heart_beat_number)
    cycle_count += ((uint64_t)1)<<32  * tf->cur_heart_beat_number;
  lEventTotalCycle=cycle_count-(tf->a_block_header->cycle_count & 0xFFFFFFFF);

  /* Convert it to nsecs */
  lEventNSec = lEventTotalCycle / tf->cycle_per_nsec;
  
  /* Determine offset in struct ltt_time */
  lTimeOffset.tv_nsec = (long)lEventNSec % 1000000000;
  lTimeOffset.tv_sec  = (long)lEventNSec / 1000000000;

  TimeAdd(time, tf->a_block_header->time, lTimeOffset);  

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

void setFieldsOffset(ltt_tracefile * t, ltt_eventtype * evT, void * evD)
{
  ltt_field * rootFld = evT->root_field;
  //  rootFld->base_address = evD;

  rootFld->field_size = getFieldtypeSize(t, evT, 0,0,rootFld, evD);  
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

int getFieldtypeSize(ltt_tracefile * t, ltt_eventtype * evT, int offsetRoot,
		     int offsetParent, ltt_field * fld, void * evD)
{
  int size, size1, element_number, i, offset1, offset2;
  ltt_type * type = fld->field_type;

  if(evT->latest_block==t->which_block && evT->latest_event==t->which_event){
    return fld->field_size;
  } 

  if(fld->field_fixed == 1){
    if(fld == evT->root_field) return fld->field_size;
  }     

  if(type->type_class != LTT_STRUCT && type->type_class != LTT_ARRAY &&
     type->type_class != LTT_SEQUENCE && type->type_class != LTT_STRING){
    if(fld->field_fixed == -1){
      size = (int) ltt_type_size(t, type);
      fld->field_fixed = 1;
    }else size = fld->field_size;

  }else if(type->type_class == LTT_ARRAY){
    element_number = (int) type->element_number;
    if(fld->field_fixed == -1){
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL);
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
				fld->child[0], evD+size);
      }      
    }else size = fld->field_size;

  }else if(type->type_class == LTT_SEQUENCE){
    size1 = (int) ltt_type_size(t, type);
    if(fld->field_fixed == -1){
      fld->field_fixed = 0;
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL);      
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
				   fld->child[0], evD+size+size1);
	}	
      }
      size += size1;
    }

  }else if(type->type_class == LTT_STRING){
    size = 0;
    if(fld->field_fixed == -1){
      fld->field_fixed = 0;
    }else{//0: string
      size = sizeof((char*)evD) + 1; //include end : '\0'
    }

  }else if(type->type_class == LTT_STRUCT){
    element_number = (int) type->element_number;
    size = 0;
    if(fld->field_fixed == -1){      
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size1=getFieldtypeSize(t, evT,offset1,offset2, fld->child[i], NULL);
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
	size=getFieldtypeSize(t,evT,offset1,offset2,fld->child[i],evD+offset2);
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
 *    timecmp   : compare two time
 *Input params 
 *    t1        : first time
 *    t2        : second time
 *Return value
 *    int       : 0: t1 == t2; -1: t1 < t2; 1: t1 > t2
 ****************************************************************************/

int timecmp(ltt_time * t1, ltt_time * t2)
{
  ltt_time T;
  TimeSub(T, *t1, *t2);
  if(T.tv_sec == 0 && T.tv_nsec == 0) return 0;
  else if(T.tv_sec > 0 || (T.tv_sec==0 && T.tv_nsec > 0)) return 1;
  else return -1;
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
  int64_t i;
  if(size == 1)      i = *(int8_t *)evD;
  else if(size == 2) i = *(int16_t *)evD;
  else if(size == 4) i = *(int32_t *)evD;
  else if(size == 8) i = *(int64_t *)evD;
 
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

void getDataEndianType(ltt_arch_size * size, ltt_arch_endian * endian)
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

