/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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
  unsigned int size;
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
  unsigned int latest_block;       //the latest block using the event type
  unsigned int latest_event;       //the latest event using the event type
};

struct _LttEvent{
  guint16  event_id;
  guint32  time_delta;
  LttTime event_time;
  LttCycleCount event_cycle_count;
  LttTracefile * tracefile;
  void * data;               //event data
  unsigned int which_block;           //the current block of the event
  unsigned int which_event;           //the position of the event
  /* This is a workaround for fast position seek */
  void * last_event_pos;

  LttTime prev_block_end_time;       //the end time of previous block
  LttTime prev_event_time;           //the time of the previous event
  LttCycleCount pre_cycle_count;     //previous cycle count of the event
  int      count;                    //the number of overflow of cycle count
  gint64 overflow_nsec;              //precalculated nsec for overflows
  TimeHeartbeat * last_heartbeat;    //last heartbeat

  /* end of workaround */
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


struct _LttFacility{
  char * name;               //facility name 
  unsigned int event_number;          //number of events in the facility 
  LttChecksum checksum;      //checksum of the facility 
  guint32  base_id;          //base id of the facility
  LttEventType ** events;    //array of event types 
  LttType ** named_types;
  unsigned int named_types_number;
};

struct _LttTracefile{
  char * name;                       //tracefile name
  LttTrace * trace;                  //trace containing the tracefile
  int fd;                            //file descriptor 
  off_t file_size;                   //file size
  unsigned block_size;               //block_size
  unsigned int block_number;         //number of blocks in the file
  unsigned int which_block;          //which block the current block is
  unsigned int which_event;          //which event of the current block 
                                     //is currently processed 
  LttTime current_event_time;        //time of the current event
  BlockStart * a_block_start;        //block start of the block
  BlockEnd   * a_block_end;          //block end of the block
  TimeHeartbeat * last_heartbeat;    //last heartbeat
  void * cur_event_pos;              //the position of the current event
  void * buffer;                     //the buffer containing the block
  double nsec_per_cycle;             //Nsec per cycle
  guint64 one_overflow_nsec;         //nsec for one overflow
  gint64 overflow_nsec;              //precalculated nsec for overflows
                                     //can be negative to include value
                                     //of block start cycle count.
                                     //incremented at each overflow while
                                     //reading.
  //LttCycleCount cycles_per_nsec_reciprocal; // Optimisation for speed
  unsigned cur_heart_beat_number;    //current number of heart beat in the buf
  LttCycleCount cur_cycle_count;     //current cycle count of the event
  void * last_event_pos;

  LttTime prev_block_end_time;       //the end time of previous block
  LttTime prev_event_time;           //the time of the previous event
  LttCycleCount pre_cycle_count;     //previous cycle count of the event
  unsigned int      count;           //the number of overflow of cycle count
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
  /* This is a workaround for fast position seek */
  void * last_event_pos;

  LttTime prev_block_end_time;       //the end time of previous block
  LttTime prev_event_time;           //the time of the previous event
  LttCycleCount pre_cycle_count;     //previous cycle count of the event
  int      count;                    //the number of overflow of cycle count
  gint64 overflow_nsec;              //precalculated nsec for overflows
  TimeHeartbeat * last_heartbeat;    //last heartbeat
  /* end of workaround */
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
