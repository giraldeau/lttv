#ifndef LTT_PRIVATE_H
#define LTT_PRIVATE_H

#include <glib.h>
#include <sys/types.h>
#include <ltt/ltt.h>

#define LTT_PACKED_STRUCT __attribute__ ((packed))

/* enumeration definition */

typedef enum _BuildinEvent{
  TRACE_FACILITY_LOAD = 0,
  TRACE_BLOCK_START   = 17,
  TRACE_BLOCK_END     = 18,
  TRACE_TIME_HEARTBEAT= 19
} BuildinEvent;


/* structure definition */

typedef struct _FacilityLoad{
  char * name;
  LttChecksum checksum;
  guint32     base_code;
} LTT_PACKED_STRUCT FacilityLoad;

typedef struct _BlockStart {
  LttTime       time;	     //Time stamp of this block
  LttCycleCount cycle_count; //cycle count of the event
  guint32       block_id;    //block id 
} LTT_PACKED_STRUCT BlockStart;

typedef struct _BlockEnd {
  LttTime       time;	     //Time stamp of this block
  LttCycleCount cycle_count; //cycle count of the event
  guint32       block_id;    //block id 
} LTT_PACKED_STRUCT BlockEnd;

typedef struct _TimeHeartbeat {
  LttTime       time;	     //Time stamp of this block
  LttCycleCount cycle_count; //cycle count of the event
} LTT_PACKED_STRUCT TimeHeartbeat;


struct _LttType{
  char * type_name;                //type name if it is a named type
  char * element_name;             //elements name of the struct
  char * fmt;
  int size;
  LttTypeEnum type_class;          //which type
  char ** enum_strings;            //for enum labels
  struct _LttType ** element_type; //for array, sequence and struct
  unsigned element_number;         //the number of elements 
                                   //for enum, array, sequence and structure
};

struct _LttEventType{
  char * name;
  char * description;
  int index;              //id of the event type within the facility
  LttFacility * facility; //the facility that contains the event type
  LttField * root_field;  //root field
  int latest_block;       //the latest block using the event type
  int latest_event;       //the latest event using the event type
};

struct _LttField{
  unsigned field_pos;        //field position within its parent
  LttType * field_type;      //field type, if it is root field
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
  int sequ_number_size;      //the size of unsigned used to save the
                             //number of elements in the sequence

  int element_size;          //the element size of the sequence
  int field_fixed;           //0: field has string or sequence
                             //1: field has no string or sequenc
                             //-1: uninitialize

  struct _LttField * parent;
  struct _LttField ** child; //for array, sequence and struct: 
                             //list of fields, it may have only one
                             //field if the element is not a struct 
  unsigned current_element;  //which element is currently processed
};

struct _LttEvent{
  guint16  event_id;
  guint32  time_delta;
  LttTime event_time;
  LttCycleCount event_cycle_count;
  LttTracefile * tracefile;
  void * data;               //event data
  int which_block;           //the current block of the event
  int which_event;           //the position of the event
};

struct _LttFacility{
  char * name;               //facility name 
  int event_number;          //number of events in the facility 
  LttChecksum checksum;      //checksum of the facility 
  guint32  base_id;          //base id of the facility
  LttEventType ** events;    //array of event types 
  LttType ** named_types;
  int  named_types_number;
};

struct _LttTracefile{
  char * name;                       //tracefile name
  LttTrace * trace;                  //trace containing the tracefile
  int fd;                            //file descriptor 
  off_t file_size;                   //file size
  unsigned block_size;               //block_size
  int block_number;                  //number of blocks in the file
  int which_block;                   //which block the current block is
  int which_event;                   //which event of the current block 
                                     //is currently processed 
  LttTime current_event_time;        //time of the current event
  BlockStart * a_block_start;        //block start of the block
  BlockEnd   * a_block_end;          //block end of the block
  void * cur_event_pos;              //the position of the current event
  void * buffer;                     //the buffer containing the block
  double cycle_per_nsec;             //Cycles per nsec
  unsigned cur_heart_beat_number;    //current number of heart beat in the buf
  LttCycleCount cur_cycle_count;     //current cycle count of the event
  void * last_event_pos;

  LttTime prev_block_end_time;       //the end time of previous block
  LttTime prev_event_time;           //the time of the previous event
  LttEvent an_event;
  LttCycleCount pre_cycle_count;     //previous cycle count of the event
  int      count;                    //the number of overflow of cycle count
};

struct _LttTrace{
  char * pathname;                          //the pathname of the trace
  guint facility_number;                    //the number of facilities 
  guint control_tracefile_number;           //the number of control files 
  guint per_cpu_tracefile_number;           //the number of per cpu files 
  LttSystemDescription * system_description;//system description 

  GPtrArray *control_tracefiles;            //array of control tracefiles 
  GPtrArray *per_cpu_tracefiles;            //array of per cpu tracefiles 
  GPtrArray *facilities;                    //array of facilities 
  LttArchSize my_arch_size;                 //data size of the local machine
  LttArchEndian my_arch_endian;             //endian type of the local machine
};

struct _LttEventPosition{
  unsigned      block_num;          //block which contains the event 
  unsigned      event_num;          //event index in the block
  unsigned      event_offset;       //event position in the block
  LttTime       event_time;         //the time of the event
  LttCycleCount event_cycle_count;  //the cycle count of the event
  unsigned      heart_beat_number;  //current number of heart beats  
  LttTracefile *tf;                 //tracefile containing the event
  gboolean      old_position;       //flag to show if it is the position
                                    //being remembered
};

/* The characteristics of the system on which the trace was obtained
   is described in a LttSystemDescription structure. */

struct _LttSystemDescription {
  char *description;
  char *node_name;
  char *domain_name;
  unsigned nb_cpu;
  LttArchSize size;
  LttArchEndian endian;
  char *kernel_name;
  char *kernel_release;
  char *kernel_version;
  char *machine;
  char *processor;
  char *hardware_platform;
  char *operating_system;
  unsigned ltt_major_version;
  unsigned ltt_minor_version;
  unsigned ltt_block_size;
  LttTime trace_start;
  LttTime trace_end;
};

/*****************************************************************************
 macro for size of some data types
 *****************************************************************************/
#define EVENT_ID_SIZE     sizeof(guint16)
#define TIME_DELTA_SIZE   sizeof(guint32)
#define EVENT_HEADER_SIZE (EVENT_ID_SIZE + TIME_DELTA_SIZE)


#endif /* LTT_PRIVATE_H */
