#define LTT_PACKED_STRUCT __attribute__ ((packed))
#define TRACER_MAGIC_NUMBER     0x00D6B7ED	/* That day marks an important historical event ... */

void initFacilities();
void freeFacilities(); //not implimented yet


//following part is for event reading API
typedef struct _trace_header_event {
  //information of the machine type
  uint32_t arch_type;	     /* Type of architecture */
  uint32_t arch_variant;     /* Variant of the given type of architecture */
  uint32_t system_type;	     /* Operating system type */
  uint32_t magic_number;     /* Magic number to identify a trace */ 

  //format of fields
  uint8_t  time_size;        /* time size */
  uint8_t  time_granul;      /* time granularity */
  uint8_t  id_size;          /* size of combined facility/event ids */

  //other elements
  uint32_t ip_addr;          /* IP of the machine */
  uint8_t  nbCpu;            /* number of CPU */
  uint8_t  cpuID;            /* cpu id */
  uint32_t buffer_size;	     /* Size of blocks */
} LTT_PACKED_STRUCT trace_header_event;


typedef struct _block_header {
  struct timeval time;	     /* Time stamp of this block */
  uint32_t tsc;              /* TSC of this block, if applicable */
  uint32_t time_delta;       /* Time between now and prev event */
  uint32_t event_count;      /* event count */
} LTT_PACKED_STRUCT block_header;


typedef struct _block_footer {
  uint32_t unused_bytes;     /* unused bytes at the end of the block */
  struct timeval time;	     /* Time stamp of this block */
  uint32_t tsc;              /* TSC of this block, if applicable */
  uint32_t time_delta;       /* Time between now and prev event */
} LTT_PACKED_STRUCT block_footer;


typedef struct _ltt_descriptor{
  int fd;                              /* file descriptor */
  off_t file_size;                     /* file size */
  int nbBlocks;                        /* number of blocks in the file */
  int which_block;                     /* which block the current block is */
  int which_event;                     /* which event of the current block is currently processed */
  uint32_t current_event_time;         /* time of the current event */
  trace_header_event * trace_header;   /* the first event in the first block */
  block_header * a_block_header;       /* block header of the block*/
  block_footer * a_block_footer;       /* block footer of the block*/
  void * first_event_pos;              /* the position of the first event in blockBuf */
  void * cur_event_pos;                /* the position of the current event in blockBuf */
  void * buffer;                       /* the buffer to contain the content of the block */ 
  int byte_rev;                        /* Byte-reverse trace data */
  double TSCPerUsec;                   /* Cycles per usec */ 
} ltt_descriptor;

typedef struct _field_handle{
  char * name;               /* field name */
  data_type field_type;      /* field type : integer, float, string, enum, array, sequence */
  int size;                  /* number of bytes for a primitive type */
  data_type element_type;    /* element type for array or sequence */
  int nbElements;            /* number of fields for a struct, and of elements for array or sequence */
  int sequence_size;         /* the length size of uint which stores the number of elements of the sequence */ 
  char * fmt;                /* printf format string for primitive type */
  off_t offset;              /* offset from the beginning of the current event */
  off_t end_field;           /* end of the field: offset of the next field */
} field_handle;

typedef struct _event_handle{
  char * name;               /* event name */
  int id;                    /* event code */
  int size_fixed;            /* indicate whether or not the event has string or sequence */
  sequence base_field;       /* base field */
  int latest_block;          /* the latest block which uses the event handle */
  int latest_event;          /* the latest event which uses the event handle */
} event_handle;


typedef struct _facility_handle{
  char * name;               /* facility name */
  int nbEvents;              /* number of events in the facility */
  unsigned long checksum;    /* checksum of the facility */
  event_handle ** events;    /* array of event types */
} facility_handle;


typedef struct _event_struct{
  int recid;                 /* event position in the combined log */
  struct timeval time;       /* detailed absolute time */
  uint32_t tsc;              /* TSC of this event */
  facility_handle * fHandle; /* facility handle */
  int event_id;              /* id of a event belonging to the facility */
  //  event_handle * event_type; /* event handle */
  uint32_t ip_addr;          /* IP address of the system */
  uint8_t CPU_id;            /* CPU id */
  int tid;                   /* thread id */
  int pid;                   /* process id */
  sequence * base_field;     /* base field */  
  void * data;               /* event binary data */
} event_struct;


typedef struct _kernel_facility{
  char * name;               /* kernel name */
  unsigned long checksum;    /* checksum of the facility */
  int nbEvents;              /* number of events in the facility */
  int firstId;               /* the ID of the first event of the facility */
} kernel_facility;


void parseEventAndTypeDefinition(char * facilityName);
void generateFacilityHandle(char * facName, unsigned long checksum, sequence * events);
int  getTypeSize(data_type dt, int index);


int getFacilitiesNumber(int * numFac, int * numEvents);   //get the number of the registered faciliyies
int getFacilitiesFromKernel(kernel_facility ** kFacilities);
int readFile(int fd, void * buf, size_t size, char * mesg);
int readBlock(ltt_descriptor * lttdes, int whichBlock);
void updateLttdes(ltt_descriptor * lttdes);
void getTSCPerUsec(ltt_descriptor * lttdes);
void getEventTime(ltt_descriptor * lttdes, uint32_t time_delta, struct timeval * pTime);

ltt_descriptor * trace_open_log(char * fileName);
int trace_seek(ltt_descriptor * lttdes, off_t offset, int whence);
int trace_seek_time(ltt_descriptor * lttdes, uint32_t offset, int whence);
int trace_read(ltt_descriptor * lttdes, event_struct * ev);

int trace_lookup_facility(int evId, facility_handle ** facilityHandle);
int trace_lookup_event(int event_id, facility_handle * facHandle, event_handle ** eventHandle);
int trace_lookup_field(sequence * baseField, int position, field_handle ** field);

int trace_get_char(field_handle * field, void * data);
int trace_get_uchar(field_handle * field, void * data);
unsigned long trace_get_enum(field_handle * field, void * data);
short int trace_get_short(field_handle * field, void * data);
unsigned short int trace_get_ushort(field_handle * field, void * data);
int trace_get_integer(field_handle * field, void * data);
unsigned int trace_get_uinteger(field_handle * field, void * data);
long trace_get_long(field_handle * field, void * data);
unsigned long trace_get_ulong(field_handle * field, void * data);
float trace_get_float(field_handle * field, void * data);
double trace_get_double(field_handle * field, void * data);
char * trace_get_string(field_handle * field, void * data);

int trace_get_time_block_position(ltt_descriptor * lttdes, uint32_t seekTime, int beginBlock, int endBlock);
void trace_update_basefield(ltt_descriptor * lttdes, sequence * baseField );


#define EVENT_ID_SIZE()     sizeof(int8_t)
#define TIME_DELTA_SIZE()   sizeof(uint32_t)
//event id and time delta
#define EVENT_HEADER_SIZE() (sizeof(int8_t) + sizeof(uint32_t))

/* Time operation macros */
/*  (T3 = T2 - T1) */
#define DBTimeSub(T3, T2, T1) \
do \
{\
  T3.tv_sec  = T2.tv_sec  - T1.tv_sec;  \
  T3.tv_usec = T2.tv_usec - T1.tv_usec; \
  if(T3.tv_usec < 0)\
    {\
    T3.tv_sec--;\
    T3.tv_usec += 1000000;\
    }\
} while(0)

/*  (T3 = T2 + T1) */
#define DBTimeAdd(T3, T2, T1) \
do \
{\
  T3.tv_sec  = T2.tv_sec  + T1.tv_sec;  \
  T3.tv_usec = T2.tv_usec + T1.tv_usec; \
  if(T3.tv_usec >= 1000000)\
    {\
    T3.tv_sec += T3.tv_usec / 1000000;\
    T3.tv_usec = T3.tv_usec % 1000000;\
    }\
} while(0)
