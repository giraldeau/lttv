#include <glib-2.0/glib.h>
#include "ltt/ltt.h"

/* structure definition */

typedef struct _trace_header_event {
  //information of the machine type
  uint32_t arch_type;	        /* Type of architecture */
  uint32_t arch_variant;        /* Variant of the given type of architecture */
  uint32_t system_type;	        /* Operating system type */
  char system_name[32];         /* system name */
  //  uint32_t magic_number;    /* Magic number to identify a trace */ 
  ltt_arch_size arch_size;      /* data type size */
  ltt_arch_endian arch_endian;  /* endian type : little or big */

  //format of fields
  uint8_t  time_size;        /* time size */
  uint8_t  time_granul;      /* time granularity */
  uint8_t  id_size;          /* size of combined facility/event ids */

  //other elements
  uint32_t ip_addr;          /* IP of the machine */
  uint8_t  cpu_number;       /* number of CPU */
  uint8_t  cpu_id;           /* cpu id */
  uint8_t  cpu_number_used;  /* the number of the cpu used in the tracefile */
  uint32_t buffer_size;	     /* Size of blocks */
} LTT_PACKED_STRUCT trace_header_event;


typedef struct _block_header {
  ltt_time  time;	       /* Time stamp of this block */
  ltt_cycle_count cycle_count; /* cycle count of the event */
  uint32_t event_count;        /* event count */
} LTT_PACKED_STRUCT block_header;


typedef struct _block_footer {
  uint32_t unused_bytes;       /* unused bytes at the end of the block */
  ltt_time time;	       /* Time stamp of this block */
  ltt_cycle_count cycle_count; /* cycle count of the event */
} LTT_PACKED_STRUCT block_footer;


struct _ltt_type{
  char * element_name;              //elements name of the struct or type name
  char * fmt;
  int size;
  ltt_type_enum type_class;         //which type
  char ** enum_strings;             //for enum labels
  struct _ltt_type ** element_type; //for array, sequence and struct
  unsigned element_number;          //the number of elements 
                                    //for enum, array, sequence and structure
};

struct _ltt_eventtype{
  char * name;
  char * description;
  int index;                 //id of the event type within the facility
  ltt_facility * facility;   //the facility that contains the event type
  ltt_field * root_field;    //root field
  int latest_block;          //the latest block using the event type
  int latest_event;          //the latest event using the event type
};

struct _ltt_field{
  unsigned field_pos;        //field position within its parent
  ltt_type * field_type;     //field type, if it is root field
                             //then it must be struct type

  off_t offset_root;         //offset from the root, -1:uninitialized 
  short fixed_root;          //offset fixed according to the root
                             //-1:uninitialized, 0:unfixed, 1:fixed
  off_t offset_parent;       //offset from the parent,-1:uninitialized
  short fixed_parent;        //offset fixed according to its parent
                             //-1:uninitialized, 0:unfixed, 1:fixed
  //  void * base_address;       //base address of the field  ????
  
  int  field_size;           //>0: size of the field, 
                             //0 : uncertain
                             //-1: uninitialize
  int element_size;          //the element size of the sequence
  int field_fixed;           //0: field has string or sequence
                             //1: field has no string or sequenc
                             //-1: uninitialize

  struct _ltt_field * parent;
  struct _ltt_field ** child;//for array, sequence and struct: 
                             //list of fields, it may have only one
                             //field if the element is not a struct 
  unsigned current_element;  //which element is currently processed
};

struct _ltt_event{
  int event_id;
  ltt_cycle_count cycle_count;
  ltt_tracefile * tracefile;
  void * data;                //event data
};

struct _ltt_facility{
  char * name;               //facility name 
  int event_number;          //number of events in the facility 
  ltt_checksum checksum;     //checksum of the facility 
  ltt_eventtype ** events;   //array of event types 
  unsigned usage_count;      //usage count
  table all_named_types;     //an array of named ltt_type
  sequence all_unnamed_types;//an array of unnamed ltt_type
  sequence all_fields;       //an array of fields
};

struct _ltt_tracefile{
  int fd;                            /* file descriptor */
  off_t file_size;                   /* file size */
  int block_number;                  /* number of blocks in the file */
  int which_block;                   /* which block the current block is */
  int which_event;                   /* which event of the current block 
                                        is currently processed */
  ltt_time current_event_time;       /* time of the current event */
  trace_header_event * trace_header; /* the first event in the first block */
  block_header * a_block_header;     /* block header of the block*/
  block_footer * a_block_footer;     /* block footer of the block*/
  void * first_event_pos;            /* the position of the first event */
  void * cur_event_pos;              /* the position of the current event */
  void * buffer;                     /* the buffer containing the block */ 
  double cycle_per_nsec;             /* Cycles per nsec */ 
  unsigned int cur_heart_beat_number;/* the number of heart beat so far 
                                        in the block */

  ltt_time start_time;               /* trace start time */
  ltt_time end_time;                 /* trace end time */
  ltt_time prev_block_end_time;      /* the end time of previous block */
  ltt_time prev_event_time;          /* the time of the previous event */

  int   eventtype_number;            /* the number of event type 
					in the tracefile */
  int   facility_number;             /* the number of the facility in 
					the tracefile */
  GHashTable * index_facility;       /* facility index and facility pair */
  GHashTable * facility_name;        /* name and facility pair */
  GHashTable * base_id_name;         /* facility name and base id pair*/

  GPtrArray * eventtype_event_id;    /* an array of eventtypes accessed 
					by event id */

  ltt_arch_size my_arch_size;        /* data type size of the local machine */
  ltt_arch_endian my_arch_endian;    /* endian type of the local machine */
};





/*****************************************************************************
 macro for size of some data types
 *****************************************************************************/
#define EVENT_ID_SIZE     sizeof(int8_t)
#define CYCLE_COUNT_SIZE   sizeof(ltt_cycle_count)
//event id and time delta(cycle count)
//yxx disable during the test
//#define EVENT_HEADER_SIZE (EVENT_ID_SIZE + CYCLE_COUNT_SIZE)
#define EVENT_HEADER_SIZE (EVENT_ID_SIZE + sizeof(uint32_t))




/* obtain the time of an event */
ltt_time getEventTime(ltt_tracefile * tf);

/* get the data type size and endian type of the local machine */
void getDataEndianType(ltt_arch_size * size, ltt_arch_endian * endian);


typedef struct _ptr_wrap{
  gpointer ptr;
} ptr_wrap;

