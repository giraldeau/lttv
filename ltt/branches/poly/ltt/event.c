#include <stdio.h>
#include <asm/types.h>
#include <linux/byteorder/swab.h>
#include <ltt/LTTTypes.h> 
#include "parser.h"
#include <ltt/event.h>

/*****************************************************************************
 *Function name
 *    ltt_event_refresh_fields   : refresh fields of an event 
 *Input params
 *    offsetRoot      : offset from the root
 *    offsetParent    : offset from the parrent
 *    fld             : field
 *    evD             : event data
 *Return value
 *    int             : size of the field
 ****************************************************************************/

int ltt_event_refresh_fields(int offsetRoot,int offsetParent, 
			     LttField * fld, void *evD)
{
  int size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  if(type->type_class != LTT_STRUCT && type->type_class != LTT_ARRAY &&
     type->type_class != LTT_SEQUENCE && type->type_class != LTT_STRING){
    size = fld->field_size;
  }else if(type->type_class == LTT_ARRAY){
    element_number = (int) type->element_number;
    if(fld->field_fixed == 0){// has string or sequence
      size = 0;
      for(i=0;i<element_number;i++){
	size += ltt_event_refresh_fields(offsetRoot+size,size, 
					 fld->child[0], evD+size);
      }      
    }else size = fld->field_size;
  }else if(type->type_class == LTT_SEQUENCE){
    size1 = fld->sequ_number_size;
    element_number = getIntNumber(size1,evD);
    type->element_number = element_number;
    if(fld->element_size > 0){
      size = element_number * fld->element_size;
    }else{//sequence has string or sequence
      size = 0;
      for(i=0;i<element_number;i++){
	size += ltt_event_refresh_fields(offsetRoot+size+size1,size+size1, 
					 fld->child[0], evD+size+size1);
      }	
      size += size1;
    }
  }else if(type->type_class == LTT_STRING){
    size = strlen((char*)evD) + 1; //include end : '\0'
  }else if(type->type_class == LTT_STRUCT){
    element_number = (int) type->element_number;
    if(fld->field_fixed == 0){
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size=ltt_event_refresh_fields(offset1,offset2,fld->child[i],evD+offset2);
	offset1 += size;
	offset2 += size;
      }      
      size = offset2;
    }else size = fld->field_size;
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  fld->fixed_root      = (offsetRoot==-1)   ? 0 : 1;
  fld->fixed_parent    = (offsetParent==-1) ? 0 : 1;
  fld->field_size      = size;

  return size;
}

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype_id: get event type id 
 *                            (base id + position of the event)
 *Input params
 *    e                     : an instance of an event type   
 *Return value
 *    unsigned              : event type id
 ****************************************************************************/

unsigned ltt_event_eventtype_id(LttEvent *e)
{
  return (unsigned) e->event_id;
}

/*****************************************************************************
 *Function name
 *    ltt_event_facility : get the facility of the event
 *Input params
 *    e                  : an instance of an event type   
 *Return value
 *    LttFacility *     : the facility of the event
 ****************************************************************************/

LttFacility *ltt_event_facility(LttEvent *e)
{
  LttTrace * trace = e->tracefile->trace;
  unsigned id = e->event_id;
  return ltt_trace_facility_by_id(trace,id);
}

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype : get the event type of the event
 *Input params
 *    e                   : an instance of an event type   
 *Return value
 *    LttEventType *     : the event type of the event
 ****************************************************************************/

LttEventType *ltt_event_eventtype(LttEvent *e)
{
  LttFacility* facility = ltt_event_facility(e);
  if(!facility) return NULL;
  return facility->events[e->event_id - facility->base_id];
}

/*****************************************************************************
 *Function name
 *    ltt_event_field : get the root field of the event
 *Input params
 *    e               : an instance of an event type   
 *Return value
 *    LttField *      : the root field of the event
 ****************************************************************************/

LttField *ltt_event_field(LttEvent *e)
{
  LttField * field;
  LttEventType * event_type = ltt_event_eventtype(e);
  if(!event_type) return NULL;
  field = event_type->root_field;
  if(!field) return NULL;

  //check if the field need refresh
  if(e->which_block != event_type->latest_block ||
     e->which_event != event_type->latest_event){

    event_type->latest_block = e->which_block;
    event_type->latest_event = e->which_event;
    
    if(field->field_fixed == 1)return field;

    //refresh the field
    ltt_event_refresh_fields(0, 0, field, e->data);    
  }
  return field;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the time of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttTime       : the time of the event
 ****************************************************************************/

LttTime ltt_event_time(LttEvent *e)
{
  return e->event_time;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the cycle count of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttCycleCount  : the cycle count of the event
 ****************************************************************************/

LttCycleCount ltt_event_cycle_count(LttEvent *e)
{
  return e->event_cycle_count;
}

/*****************************************************************************
 *Function name
 *    ltt_event_position : get the event's position
 *Input params
 *    e                  : an instance of an event type   
 *    ep                 : a pointer to event's position structure
 ****************************************************************************/

void ltt_event_position(LttEvent *e, LttEventPosition *ep)
{
  ep->block_num         = e->which_block;
  ep->event_num         = e->which_event;
  ep->event_time        = e->event_time;
  ep->event_cycle_count = e->event_cycle_count;
  ep->heart_beat_number = e->tracefile->cur_heart_beat_number;
  ep->old_position      = TRUE;
  ep->event_offset      = e->data - e->tracefile->buffer - EVENT_HEADER_SIZE ;
}

/*****************************************************************************
 *Function name
 *    ltt_event_position_get : get the block number and index of the event
 *Input params
 *    ep                     : a pointer to event's position structure
 *    block_number           : the block number of the event
 *    index_in_block         : the index of the event within the block
 ****************************************************************************/

void ltt_event_position_get(LttEventPosition *ep,
    unsigned *block_number, unsigned *index_in_block)
{
  *block_number   = ep->block_num;
  *index_in_block = ep->event_num;
}

/*****************************************************************************
 *Function name
 *    ltt_event_position_set : set the block number and index of the event
 *Input params
 *    ep                     : a pointer to event's position structure
 *    block_number           : the block number of the event
 *    index_in_block         : the index of the event within the block
 ****************************************************************************/

void ltt_event_position_set(LttEventPosition *ep,
    unsigned block_number, unsigned index_in_block)
{
  ep->block_num = block_number;
  ep->event_num = index_in_block;
}

/*****************************************************************************
 *Function name
 *    ltt_event_cpu_i: get the cpu id where the event happens
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    unsigned       : the cpu id
 ****************************************************************************/

unsigned ltt_event_cpu_id(LttEvent *e)
{  
  return (unsigned)atoi(e->tracefile->name);
}

/*****************************************************************************
 *Function name
 *    ltt_event_data : get the raw data for the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    void *         : pointer to the raw data for the event
 ****************************************************************************/

void *ltt_event_data(LttEvent *e)
{
  return e->data;
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_number
 *                   : The number of elements in a sequence field is specific
 *                     to each event. This function returns the number of 
 *                     elements for an array or sequence field in an event.
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *Return value
 *    unsigned       : the number of elements for an array/sequence field
 ****************************************************************************/

unsigned ltt_event_field_element_number(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return 0;
  
  if(f->field_type->type_class == LTT_ARRAY)
    return f->field_type->element_number;
  return (unsigned)  getIntNumber(f->sequ_number_size, e + f->offset_root);
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_select
 *                   : Set the currently selected element for a sequence or
 *                     array field
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *    i              : the ith element
 ****************************************************************************/

void ltt_event_field_element_select(LttEvent *e, LttField *f, unsigned i)
{
  unsigned element_number;
  LttField *fld;
  int k, size;
  void *evD;
 
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return ;

  element_number  = ltt_event_field_element_number(e,f);
  if((element_number-1) < i || i < 0) return;
  
  fld = f->child[0];
  
  evD = e->data + f->offset_root;
  size = 0;
  for(k=0;k<i;k++){
    size += ltt_event_refresh_fields(f->offset_root+size,size, fld, evD+size);
  }

  f->current_element = i - 1;
}

/*****************************************************************************
 * These functions extract data from an event after architecture specific
 * conversions
 ****************************************************************************/

unsigned ltt_event_get_unsigned(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;
  LttTypeEnum t = f->field_type->type_class;

  if(t != LTT_UINT && t != LTT_ENUM)
    g_error("The type of the field is not unsigned int\n");

  if(f->field_size == 1){
    uint8_t x = *(uint8_t*)(e->data + f->offset_root);
    return (unsigned int) x;    
  }else if(f->field_size == 2){
    uint16_t x = *(uint16_t*)(e->data + f->offset_root);
    return (unsigned int) (revFlag ? BREV16(x): x);    
  }else if(f->field_size == 4){
    uint32_t x = *(uint32_t*)(e->data + f->offset_root);
    return (unsigned int) (revFlag ? BREV32(x): x);    
  }else if(f->field_size == 8){
    uint64_t x = *(uint64_t*)(e->data + f->offset_root);
    return (unsigned int) (revFlag ? BREV64(x): x);    
  }
}

int ltt_event_get_int(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_INT)
    g_error("The type of the field is not int\n");

  if(f->field_size == 1){
    int8_t x = *(int8_t*)(e->data + f->offset_root);
    return (int) x;    
  }else if(f->field_size == 2){
    int16_t x = *(int16_t*)(e->data + f->offset_root);
    return (int) (revFlag ? BREV16(x): x);    
  }else if(f->field_size == 4){
    int32_t x = *(int32_t*)(e->data + f->offset_root);
    return (int) (revFlag ? BREV32(x): x);    
  }else if(f->field_size == 8){
    int64_t x = *(int64_t*)(e->data + f->offset_root);
    return (int) (revFlag ? BREV64(x): x);    
  }
}

unsigned long ltt_event_get_long_unsigned(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;
  LttTypeEnum t = f->field_type->type_class;

  if(t != LTT_UINT && t != LTT_ENUM)
    g_error("The type of the field is not unsigned long\n");

  if(f->field_size == 1){
    uint8_t x = *(uint8_t*)(e->data + f->offset_root);
    return (unsigned long) x;    
  }else if(f->field_size == 2){
    uint16_t x = *(uint16_t*)(e->data + f->offset_root);
    return (unsigned long) (revFlag ? BREV16(x): x);    
  }else if(f->field_size == 4){
    uint32_t x = *(uint32_t*)(e->data + f->offset_root);
    return (unsigned long) (revFlag ? BREV32(x): x);    
  }else if(f->field_size == 8){
    uint64_t x = *(uint64_t*)(e->data + f->offset_root);
    return (unsigned long) (revFlag ? BREV64(x): x);    
  }
}

long int ltt_event_get_long_int(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if( f->field_type->type_class != LTT_INT)
    g_error("The type of the field is not long int\n");

  if(f->field_size == 1){
    int8_t x = *(int8_t*)(e->data + f->offset_root);
    return (long) x;    
  }else if(f->field_size == 2){
    int16_t x = *(int16_t*)(e->data + f->offset_root);
    return (long) (revFlag ? BREV16(x): x);    
  }else if(f->field_size == 4){
    int32_t x = *(int32_t*)(e->data + f->offset_root);
    return (long) (revFlag ? BREV32(x): x);    
  }else if(f->field_size == 8){
    int64_t x = *(int64_t*)(e->data + f->offset_root);
    return (long) (revFlag ? BREV64(x): x);    
  }
}

float ltt_event_get_float(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_FLOAT || 
     (f->field_type->type_class == LTT_FLOAT && f->field_size != 4))
    g_error("The type of the field is not float\n");

  if(revFlag == 0) return *(float *)(e->data + f->offset_root);
  else{
    uint32_t aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 4);
    aInt = ___swab32(aInt);
    return *((float*)&aInt);
  }
}

double ltt_event_get_double(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_FLOAT || 
     (f->field_type->type_class == LTT_FLOAT && f->field_size != 8))
    g_error("The type of the field is not double\n");

  if(revFlag == 0) return *(double *)(e->data + f->offset_root);
  else{
    uint64_t aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 8);
    aInt = ___swab64(aInt);
    return *((double *)&aInt);
  }
}

/*****************************************************************************
 * The string obtained is only valid until the next read from
 * the same tracefile.
 ****************************************************************************/

char *ltt_event_get_string(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_STRING)
    g_error("The field contains no string\n");
  return (char*)g_strdup((char*)(e->data + f->offset_root));
}
