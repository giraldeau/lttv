#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/errno.h>  

//#include <sys/time.h>

#include "LTTTypes.h"  

#include "parser.h"
#include "EventAPI.h"

//during the test
#include "default.h"

sequence facilities;
int facilitiesNum, totalNumEvents;
kernel_facility ** idToFacility;   //corresponding between event id and kernel facility, it points to kFacilities
kernel_facility ** kFacilities;

int intSize[]   = {sizeof(int8_t),  sizeof(int16_t),   sizeof(int32_t),
                   sizeof(int64_t), sizeof(short int), sizeof(int),
                   sizeof(long) };
int uintSize[]  = {sizeof(uint8_t),  sizeof(uint16_t),   sizeof(uint32_t),
                   sizeof(uint64_t), sizeof(unsigned short int), 
                   sizeof(unsigned int),  sizeof(unsigned long) };
int floatSize[] = {-1, -1, sizeof(float), sizeof(double),
                   -1,     sizeof(float), sizeof(double)};

//will be deleted later, test
char *buf[] = {"TRACE_EV_START",        "TRACE_EV_SYSCALL_ENTRY", "TRACE_EV_SYSCALL_EXIT", "TRACE_EV_TRAP_ENTRY",
               "TRACE_EV_TRAP_EXIT",    "TRACE_EV_IRQ_ENTRY",     "TRACE_EV_IRQ_EXIT",     "TRACE_EV_SCHEDCHANGE",
               "TRACE_EV_KERNEL_TIMER", "TRACE_EV_SOFT_IRQ",      "TRACE_EV_PROCESS",      "TRACE_EV_FILE_SYSTEM",
               "TRACE_EV_TIMER",        "TRACE_EV_MEMORY",        "TRACE_EV_SOCKET",       "TRACE_EV_IPC",
               "TRACE_EV_NETWORK",      "TRACE_EV_BUFFER_START",  "TRACE_EV_BUFFER_END",   "TRACE_EV_NEW_EVENT",
               "TRACE_EV_CUSTOM",       "TRACE_EV_CHANGE_MASK",   "TRACE_EV_HEARTBEAT"  };
FILE * fp;
void print_event( event_struct * ev){
  int i,j;
  sequence * baseField = ev->base_field;
  char* ch;
  field_handle * fHandle;
  void* curPos = ev->data;
  void * offset;
  data_type dt, edt;

  fprintf(fp,"Event id = %d, %s\n",ev->event_id, buf[ev->event_id]);

  for(i=0;i<baseField->position;i++){
    fHandle = (field_handle *) baseField->array[i];
    dt = fHandle->field_type;
    fprintf(fp,"%s: ", fHandle->name);
    if(dt == INT || dt == UINT || dt == FLOAT || dt == ENUM){
      if(dt == FLOAT){
	if(fHandle->size == 4)fprintf(fp, "%f\n",*(float*)(curPos+fHandle->offset));
	else   fprintf(fp, "%f\n",(float)(*(double*)(curPos+fHandle->offset)));
      }else{
	if(fHandle->size == 1)fprintf(fp, "%d\n",(int)(*(int8_t *)(curPos+fHandle->offset)));
	else if(fHandle->size == 2)fprintf(fp, "%d\n",(int)(*(int16_t *)(curPos+fHandle->offset)));
	else if(fHandle->size == 4)fprintf(fp, "%d\n",(int)(*(int32_t *)(curPos+fHandle->offset)));
	else if(fHandle->size == 8)fprintf(fp, "%d\n",(int)(*(int64_t *)(curPos+fHandle->offset)));
      }
    }else if(dt == ARRAY || dt == SEQUENCE ){
      ch = (char*)(curPos+fHandle->offset);
      offset = curPos+fHandle->offset;
      for(j=0;j<fHandle->nbElements;j++){
	edt = fHandle->element_type;
	if(edt == INT || edt == UINT || edt == FLOAT || edt == ENUM){
	  if(edt == FLOAT){
	    if(fHandle->size == 4)fprintf(fp, "%f, ",*(float*)offset);
	    else   fprintf(fp, "%f\n",(float)(*(double*)offset));
	  }else{
	    if(fHandle->size == 1)fprintf(fp, "%d, ",(int)(*(int8_t *)offset));
	    else if(fHandle->size == 2)fprintf(fp, "%d, ",(int)(*(int16_t *)offset));
	    else if(fHandle->size == 4)fprintf(fp, "%d, ",(int)(*(int32_t *)offset));
	    else if(fHandle->size == 8)fprintf(fp, "%d, ",(int)(*(int64_t *)offset));
	  }
	  offset += fHandle->size;
	}else if(edt == STRING){
	  fprintf(fp,"%s, ", ch);
	  while(1){
	    if(*ch == '\0'){ch++; break;}    //string ended with '\0'	
	    ch++;
	  }
	}else if(edt == ARRAY || edt == SEQUENCE || edt == STRUCT){ // not supported
	}
      }
      fprintf(fp,"\n");
    }else if(dt == STRING){
      fprintf(fp,"%s\n", (char*)(curPos+fHandle->offset));      
    }else if (dt == STRUCT){ //not supported
    }
  }

  fprintf(fp,"\n\n");
  //  fflush(fp);
}
//end

/*****************************************************************************
 *Function name
 * parseEventAndTypeDefinition: main function to parse event and type definition 
 ****************************************************************************/
void parseEventAndTypeDefinition(char * facilityName){
  char *token;
  parse_file in;
  char buffer[BUFFER_SIZE];
  unsigned long checksum;
  event *ev;
  sequence  events;
  table namedTypes;
  sequence unnamedTypes;

  sequence_init(&events);
  table_init(&namedTypes);
  sequence_init(&unnamedTypes);

  in.buffer = buffer;
  in.lineno = 0;
  in.error = error_callback;
  in.name = appendString(facilityName,".event");

  in.fp = fopen(in.name, "r");
  if(!in.fp )in.error(&in,"cannot open input file");
 
  while(1){
    token = getToken(&in);
    if(in.type == ENDFILE) break;
    
    if(strcmp("event",token) == 0) {
      ev = (event *)memAlloc(sizeof(event));
      sequence_push(&events,ev);
      parseEvent(&in,ev, &unnamedTypes, &namedTypes);
    }
    else if(strcmp("type",token) == 0) {
      parseTypeDefinition(&in, &unnamedTypes, &namedTypes);
    }
    else in.error(&in,"event or type token expected");
  }
  
  fclose(in.fp);

  checkNamedTypesImplemented(&namedTypes);

  generateChecksum(facilityName, &checksum, &events);

  generateFacilityHandle(facilityName,checksum,&events);


  free(in.name);
  freeEvents(&events);
  sequence_dispose(&events);
  freeNamedType(&namedTypes);
  table_dispose(&namedTypes);
  freeTypes(&unnamedTypes);
  sequence_dispose(&unnamedTypes);

}

/*****************************************************************************
 *Function name
 *    generateFacilityHandle    : generate facility handle 
 *Input params 
 *    facName                   : facility name
 *    checksum                  : checksum of the facility          
 *    events                    : sequence of events                     
 ****************************************************************************/
void  generateFacilityHandle(char * facName, unsigned long checksum, sequence * events){
  int i,j, pos,nbEvents=0;
  facility_handle * facHandle;
  event_handle * evHandle;
  field_handle * fldHandle;
  event * ev;
  field *fld;
  data_type dt;
  off_t offset;

  //get number of events which do not have nested struct
  for(i=0;i<events->position;i++){
    ev = (event *)events->array[i];
    if(ev->nested)continue;
    nbEvents++;
  }

  //allocate memory for the facility
  facHandle = (facility_handle *) memAlloc(sizeof(facility_handle));
  facHandle->name = allocAndCopy(facName);
  facHandle->checksum = checksum;
  facHandle->nbEvents = events->position;
  facHandle->events = (event_handle **)memAlloc(sizeof(event_handle*) * nbEvents);
  
  //allocate memory for event handles and associate them with the facility
  evHandle = (event_handle *)memAlloc(sizeof(event_handle) * nbEvents);
  for(i=0;i<nbEvents;i++){
    sequence_init(&evHandle[i].base_field);
    facHandle->events[i] = &evHandle[i];
  }

  j = -1;
  for(pos=0;pos<events->position;pos++){
    ev = (event *)events->array[pos];
    if(ev->nested)continue;
    j++;
    evHandle[j].name = allocAndCopy(ev->name);
    evHandle[j].id = pos;
    evHandle[j].size_fixed = 1; //by default there are no string or sequence
    if(ev->type->fields.position){
      fldHandle = (field_handle *)memAlloc(sizeof(field_handle) * ev->type->fields.position);      
    }
    evHandle[j].latest_block = -1;
    evHandle[j].latest_event = -1;
    offset = EVENT_HEADER_SIZE();  //the beginning of the event is id and time delta
    for(i=0;i<ev->type->fields.position;i++){
      fld = (field *)ev->type->fields.array[i];
      fldHandle[i].name = allocAndCopy(fld->name);
      fldHandle[i].field_type = fld->type->type;
      dt = fld->type->type;
      fldHandle[i].fmt = NULL;
      fldHandle[i].element_type = NONE;
      fldHandle[i].sequence_size = -1;
      fldHandle[i].offset = -1;
      fldHandle[i].end_field = -1;
      if(dt==UINT || dt==INT || dt==FLOAT || dt==ENUM){
	fldHandle[i].size = getTypeSize(dt, fld->type->size);
	if(evHandle[j].size_fixed){
	  fldHandle[i].offset = offset;
	  offset += fldHandle[i].size;
	  fldHandle[i].end_field = offset;
	}
	if(fld->type->fmt) fldHandle[i].fmt = allocAndCopy(fld->type->fmt);
      }else if(dt==STRING){
	fldHandle[i].size = -1;	                                           //-1 means: size is not known
	if(evHandle[j].size_fixed){
	  evHandle[j].size_fixed = 0;
	  fldHandle[i].offset = offset;
	  offset = -1;
	}
	if(fld->type->fmt) fldHandle[i].fmt = allocAndCopy(fld->type->fmt);
      }else if(dt==ARRAY || dt==SEQUENCE){
	fldHandle[i].size = getTypeSize(fld->type->nested_type->type, fld->type->nested_type->size); //other primitive type
	fldHandle[i].element_type = fld->type->nested_type->type;
	if(evHandle[j].size_fixed){
	  fldHandle[i].offset = offset;
	  if(dt==SEQUENCE){
	    evHandle[j].size_fixed = 0;
	    offset = -1;
	  }else{
	    if(fldHandle[i].size > 0){
	      offset += fld->type->size * fldHandle[i].size;
	      fldHandle[i].end_field = offset;
	    }
	    else{
	      evHandle[j].size_fixed = 0;
	      offset = -1;
	    }
	  }
	}
	if(dt==ARRAY) fldHandle[i].nbElements = fld->type->size;
	else fldHandle[i].nbElements = -1;	                           //is not known
	if(dt==SEQUENCE) fldHandle[i].sequence_size = getTypeSize(UINT, fld->type->size);
      }else if(dt==STRUCT){
 	fldHandle[i].size = -1;                                            //impossible: do not support nested struct
	if(evHandle[j].size_fixed){
	  evHandle[j].size_fixed = 0;
	  offset = -1;	  
	}	
      }

      //add field handle into the sequence of field handles of the event handle
      sequence_push(&evHandle[j].base_field, &fldHandle[i]);
    }
  }

  //add the facility handle into the sequence of facilities
  sequence_push(&facilities,facHandle);  
}

/*****************************************************************************
 *Function name
 *    getTypeSize   : get the size of a given data type 
 *Input params 
 *    dt            : data type
 *    index         : index of the data type                              
 *Output params    
 *    int           : return the size of the data type   
 ****************************************************************************/
int getTypeSize(data_type dt, int index){
  int size = -1;
  switch(dt){
    case INT:
      size = intSize[index];
      break;
    case UINT:
    case ENUM:
      size = uintSize[index];
      break;
    case FLOAT:
      size = floatSize[index];
      break;
    case STRING:
    case ARRAY:
    case SEQUENCE:
    case STRUCT: 
    default:
  }
  return size;  
}

/*****************************************************************************
 *Function name
 *   initFacilities: initialize the sequence for facilities and get all of the
 *                   registered facilities from kernel
 *****************************************************************************/
void initFacilities(){
  int err, i, j;

  sequence_init(&facilities);

  //get the number of the registered facilities from kernel
  err = getFacilitiesNumber( &facilitiesNum, &totalNumEvents);
  if(err){
    printf("Unable to get the number of the registered facilities from kernel\n");
    exit(1);
  }

  //get all of the registered facilities from kernel
  kFacilities = (kernel_facility**)memAlloc(sizeof(kernel_facility*) * facilitiesNum);
  for(i=0;i<facilitiesNum;i++){
    kFacilities[i] = (kernel_facility *)memAlloc(sizeof(kernel_facility));
  }
  err = getFacilitiesFromKernel(kFacilities);
  if(err){
    printf("Unable to get all of the registered facilities from kernel\n");
    exit(1);
  }

  //set up the relation between event id and kernel facility
  idToFacility = (kernel_facility**)memAlloc(sizeof(kernel_facility*) * totalNumEvents);
  for(i=0;i<facilitiesNum;i++){
    for(j=kFacilities[i]->firstId; j < kFacilities[i]->firstId + kFacilities[i]->nbEvents; j++){
      idToFacility[j] = kFacilities[i];
    }
  }
  
}


/*****************************************************************************
 *Function name
 *    trace_open_log    : open trace files
 *Input Params
 *    fileName          : trace file name 
 *Output Params
 *    ltt_descriptor    : handle to opened files
 ****************************************************************************/
ltt_descriptor * trace_open_log(char * fileName){
  ltt_descriptor * lttdes;
  struct stat      lTDFStat;    /* Trace data file status */
  trace_header_event *trace_header;
  block_header   a_block_header;

  //initialize the sequence for facilities and get all registered facilities from kernel
  initFacilities();

  lttdes = (ltt_descriptor *)memAlloc(sizeof(ltt_descriptor));

  //open the file
  lttdes->fd = open(fileName, O_RDONLY, 0);
  if(lttdes->fd < 0){
    printf("Unable to open input data file %s\n", fileName);
    exit(1);
  }
 
  // Get the file's status 
  if(fstat(lttdes->fd, &lTDFStat) < 0){
    printf("Unable to get the status of the input data file %s\n", fileName);
    exit(1);    
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < sizeof(block_header) + sizeof(trace_header_event)){
    printf("The input data file %s does not contain a trace\n", fileName);
    exit(1);    
  }
  
  //store the size of the file
  lttdes->file_size = lTDFStat.st_size;

  //read the first block header
  if(readFile(lttdes->fd,(void*)&a_block_header, sizeof(block_header), "Unable to read block header"))
    exit(1);
  
  //read the trace header event
  trace_header = memAlloc(sizeof(trace_header_event));
  if(readFile(lttdes->fd,(void*)trace_header, sizeof(trace_header_event),"Unable to read trace header event"))
    exit(1);

  lttdes->nbBlocks = lttdes->file_size / trace_header->buffer_size;
  lttdes->trace_header = (trace_header_event *)trace_header;
  
  //read the first block
  lttdes->buffer = memAlloc(trace_header->buffer_size);
  lseek(lttdes->fd, 0,SEEK_SET);
  if(readBlock(lttdes,1)) exit(1);

  if(lttdes->trace_header->magic_number == TRACER_MAGIC_NUMBER) lttdes->byte_rev = 0;
  else if(BREV32(lttdes->trace_header->magic_number) == TRACER_MAGIC_NUMBER) lttdes->byte_rev = 1;
  else exit(1);
  


  return lttdes;
}

/*****************************************************************************
 *Function name
 *    getFacilitiesNumber      : get the number of the registered facilities 
 *                               and the total number of events from kernel
 *Input Params
 *    facNum                   : point to the number of registered facilities
 *return value 
 *    0                        : success
 ****************************************************************************/
int getFacilitiesNumber(int * numFac, int * numEvents){
  //not implemented yet
  return 0;
}

/*****************************************************************************
 *Function name
 *    getFacilitiesFromKernel  : get all the registered facilities from kernel
 *Input Params
 *    kFacilities              : an array of kernel facility structure
 *return value 
 *    0                        : success
 ****************************************************************************/
int getFacilitiesFromKernel(kernel_facility ** kFacilities){
  //not implemented yet

  return 0;
}

/*****************************************************************************
 *Function name
 *    getTSCPerUsec : calculate cycles per Usec for current block
 *Input Params
 *    lttdes      : ltt file descriptor
 ****************************************************************************/
void getTSCPerUsec(ltt_descriptor * lttdes){
  struct timeval      lBufTotalTime; /* Total time for this buffer */
  uint32_t            lBufTotalUSec; /* Total time for this buffer in usecs */
  uint32_t            lBufTotalTSC;  /* Total TSC cycles for this buffer */

  /* Calculate the total time for this buffer */
  DBTimeSub(lBufTotalTime,lttdes->a_block_footer->time, lttdes->a_block_header->time);
  /* Calculate the total cycles for this bufffer */
  lBufTotalTSC = lttdes->a_block_footer->tsc - lttdes->a_block_header->tsc;
  /* Convert the total time to usecs */
  lBufTotalUSec = lBufTotalTime.tv_sec * 1000000 + lBufTotalTime.tv_usec;
  
  lttdes->TSCPerUsec = (double)lBufTotalTSC / (double)lBufTotalUSec;
}

/*****************************************************************************
 *Function name
 *    getEventTime : calculate the time the event occurs
 *Input Params
 *    lttdes      : ltt file descriptor
 *    time_delta  : time difference between block header and the event
 *Output Params
 *    pTime       : pointer to event time
 ****************************************************************************/
void  getEventTime(ltt_descriptor * lttdes, uint32_t time_delta, struct timeval * pTime){
  uint32_t       lEventTotalTSC;/* Total cycles from start for event */
  double         lEventUSec;    /* Total usecs from start for event */
  struct timeval lTimeOffset;   /* Time offset in struct timeval */
  
  /* Calculate total time in TSCs from start of buffer for this event */
  lEventTotalTSC = time_delta - lttdes->a_block_header->tsc;
  /* Convert it to usecs */
  lEventUSec = lEventTotalTSC/lttdes->TSCPerUsec;
  
  /* Determine offset in struct timeval */
  lTimeOffset.tv_usec = (long)lEventUSec % 1000000;
  lTimeOffset.tv_sec  = (long)lEventUSec / 1000000;

  DBTimeAdd((*pTime), lttdes->a_block_header->time, lTimeOffset);  
}


/*****************************************************************************
 *Function name
 *    trace_read  : get an event from the file
 *Input Params
 *    lttdes      : ltt file descriptor
 *    ev          : a structure contains the event
 *return value 
 *    0           : success
 *    ENOENT      : end of the file
 *    EIO         : can not read from the file
 *    EINVAL      : registered facility is not the same one used here
 ****************************************************************************/
int trace_read(ltt_descriptor * lttdes, event_struct * ev){
  int err;
  int8_t evId;
  facility_handle * facHandle;
  uint32_t time_delta;
  event_handle * eHandle;
  int8_t * tmp;

  //check if it is the end of a block and if it is the end of the file
  if(lttdes->which_event == lttdes->a_block_header->event_count){
    if(lttdes->which_block == lttdes->nbBlocks ){
      return ENOENT; //end of file
    }else{
      if((err=readBlock(lttdes, lttdes->which_block + 1)) != 0) return err;
    }
  }
  
  //get the event ID and time
  evId = *((int8_t*)lttdes->cur_event_pos);
  time_delta = *((uint32_t *)(lttdes->cur_event_pos + EVENT_ID_SIZE()));

  //get facility handle
  err = trace_lookup_facility(evId, &facHandle);
  if(err)return err;

  lttdes->current_event_time = time_delta;

  //ev->recid = ??;
  getEventTime(lttdes, time_delta, &(ev->time));
  ev->tsc = time_delta;
  ev->fHandle = facHandle;

  //  ev->event_id = evId - idToFacility[evId]->firstId; /* disable during the test */
  ev->event_id = evId;

  ev->ip_addr = lttdes->trace_header->ip_addr;
  ev->CPU_id = lttdes->trace_header->cpuID;
  //  ev->tid = ??;
  //  ev->pid = ??;
  ev->base_field = &(ev->fHandle->events[ev->event_id]->base_field);
  ev->data = lttdes->cur_event_pos;

  //if there are strings or sequence in the event, update base field
  eHandle = ev->fHandle->events[ev->event_id];
  if(eHandle->size_fixed == 0 && (eHandle->latest_block != lttdes->which_block || eHandle->latest_event != lttdes->which_event)){
    if(ev->base_field->position == 0) lttdes->cur_event_pos += EVENT_HEADER_SIZE();
    else trace_update_basefield(lttdes, ev->base_field);
    eHandle->latest_block = lttdes->which_block;
    eHandle->latest_event = lttdes->which_event;
  }
  else {
    if(ev->base_field->position == 0) lttdes->cur_event_pos += EVENT_HEADER_SIZE();
    else{
      //will be deleted late, test
      tmp = (int8_t *)lttdes->cur_event_pos;
      for(err=0;err <((field_handle *)ev->base_field->array[ev->base_field->position-1])->end_field;err++){
	evId = *tmp;
	tmp++;
      }
      //end

      lttdes->cur_event_pos += ((field_handle *)ev->base_field->array[ev->base_field->position-1])->end_field;
    }
  }
  

  lttdes->which_event++;

  //will be delete late, test
  //  print_event( ev);

  return 0;
}

/*****************************************************************************
 *Function name
 *    trace_update_basefield  : update base fields
 *Input Params
 *    lttdes      : ltt file descriptor
 *    baseField   : base field of the event
 ****************************************************************************/
void trace_update_basefield(ltt_descriptor * lttdes, sequence * baseField){
  int i,j;
  char ch;
  field_handle * fHandle;
  void* curPos ;
  off_t offset = EVENT_HEADER_SIZE(); // id and time delta
  data_type dt, edt;
  int fixSize = 1; //by default, no string or sequence

  for(i=0;i<baseField->position;i++){
    fHandle = (field_handle *) baseField->array[i];
    dt = fHandle->field_type;
    curPos = lttdes->cur_event_pos + offset;
    if(dt == INT || dt == UINT || dt == FLOAT || dt == ENUM){
      if(!fixSize) fHandle->offset = offset;
      offset += fHandle->size;
      if(!fixSize) fHandle->end_field = offset;
    }else if(dt == ARRAY || dt == SEQUENCE ){	
      edt = fHandle->element_type;
      if(dt == SEQUENCE){
	fixSize = 0;
	if(fHandle->sequence_size== 1)fHandle->nbElements = (int)(*(uint8_t *)(curPos));
	else if(fHandle->sequence_size== 2)fHandle->nbElements = (int)(*(uint16_t *)(curPos));
	else if(fHandle->sequence_size== 3)fHandle->nbElements = (int)(*(uint32_t *)(curPos));
	else if(fHandle->sequence_size== 4)fHandle->nbElements = (int)(*(unsigned long *)(curPos)); //
      }
      if(edt == INT || edt == UINT || edt == FLOAT || edt == ENUM){
	if(!fixSize) fHandle->offset = offset;
	if(dt == SEQUENCE) offset += fHandle->sequence_size;
	offset += fHandle->nbElements * fHandle->size;
	if(!fixSize) fHandle->end_field = offset;
      }else if(edt == STRING){
	fHandle->offset = offset;
	if(dt ==SEQUENCE){
	  curPos += fHandle->sequence_size;
	  offset += fHandle->sequence_size;
	}
	if(fixSize) fixSize = 0;
	for(j=0;j<fHandle->nbElements;j++){
	  while(1){
	    ch = *(char*)(curPos);
	    offset++;
	    curPos++;
	    if(ch == '\0') break;    //string ended with '\0'	
	  }
	}
	fHandle->end_field = offset;
      }else if(edt == ARRAY || edt == SEQUENCE || edt == STRUCT){ // not supported
      }
    }else if(dt == STRING){
      if(fixSize) fixSize = 0;
      fHandle->offset = offset;
      while(1){
	ch = *(char*)(curPos);
	offset++;
	curPos++;
	if(ch == '\0') break;    //string ended with '\0'	
      }
      fHandle->end_field = offset;
    }else if (dt == STRUCT){ //not supported
    }
  }

  //  lttdes->cur_event_pos = (void*)(&fHandle->end_field);
  lttdes->cur_event_pos += fHandle->end_field;
}

/****************************************************************************
 *Function name
 *    trace_lookup_facility: get the facility handle
 *Input Params
 *    facilityName         : facility name
 *Output Params
 *    facHandle            : facility handle
 *return value 
 *    0                    : success
 *    EINVAL               : registered facility is not the same one used here
 ****************************************************************************/
int trace_lookup_facility(int evId, facility_handle ** facHandle){
  int i;
  facility_handle * fHandle;

  for(i=0;i<facilities.position;i++){
    fHandle = (facility_handle *)facilities.array[i];

    /* disable during the test */
    *facHandle = fHandle;
    return 0;
/*
    if(strcmp(idToFacility[evId]->name, fHandle->name)==0){
      if(idToFacility[evId]->checksum != fHandle->checksum) 
	return EINVAL;
      *facHandle = fHandle;
      return 0;
    }
*/
  }

  //read event definition file
  //  parseEventAndTypeDefinition(idToFacility[evId]->name);  / * disable during the test */
  parseEventAndTypeDefinition("default");  
  fHandle = (facility_handle *)facilities.array[facilities.position-1];

  //check if the facility used here is the same as the one registered in kernel
  //  if(idToFacility[evId]->checksum != fHandle->checksum) return EINVAL;  /* disable during the test */

  *facHandle = fHandle;

  return 0;
}

/****************************************************************************
 *Function name
 *    trace_lookup_event: get the event handle
 *Input Params
 *    event_id          : id of event in the facility
 *    facHandle         : facility handle containing an array of event handle
 *Output Params
 *    eventHandle       : event handle
 *return value 
 *    0           : success
 ****************************************************************************/
int trace_lookup_event(int event_id, facility_handle * facHandle, event_handle ** eventHandle){
  *eventHandle = facHandle->events[event_id];
  return 0;
}

/****************************************************************************
 *Function name
 *    trace_lookup_field: get field handle
 *Input Params
 *    baseField         : a sequence of field handles
 *    position          : the position of the field handle in the baseField
 *Output Params 
 *    fieldHandle       : field handle
 *return value 
 *    0                 : success
 *    EINVAL            : not a valid position
 ****************************************************************************/
int trace_lookup_field(sequence * baseField, int position, field_handle ** fieldHandle){  
  if(position >= baseField->position || position < 0) return EINVAL;
  *fieldHandle = (field_handle*)baseField->array[position];
  return 0;
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
int readFile(int fd, void * buf, size_t size, char * mesg){
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
 *    trace_seek      : seek according to event
 *Input Params
 *    lttdes          : ltt file descriptor
 *    offset          : the file offset : event
 *    whence          : how to set offset, it can be SEEK_SET, SEEK_CUR, SEEK_END
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 *    ENOENT          : end of file
 ****************************************************************************/
int trace_seek(ltt_descriptor * lttdes, off_t offset, int whence){
  off_t newOffset = offset;
  int blockNum, i,j, err;
  event_struct ev;

  if(whence == SEEK_CUR && offset == 0)return 0;

  if(whence == SEEK_SET){
    if(offset < 0) return EINVAL;
    if(lttdes->which_block != 1){
      if((err=readBlock(lttdes, 1)) != 0) return err;
    }else updateLttdes(lttdes);
  }else if(whence == SEEK_END){
    if(offset > 0) return EINVAL;
    if(lttdes->which_block != lttdes->nbBlocks){
      if((err=readBlock(lttdes, lttdes->nbBlocks))!=0) return err;
    }else updateLttdes(lttdes);    
    newOffset = lttdes->a_block_header->event_count + offset;    
    if(newOffset > (off_t)lttdes->a_block_header->event_count ) return EINVAL;
  }else{ //whence == SEEK_CUR
    if(offset < 0){
      newOffset = lttdes->which_event + offset;
      updateLttdes(lttdes);
      if(lttdes->which_block == 1){
	newOffset--;
	if(newOffset < 0) return 0; //at the beginning of the file
      }      
    }
  }

  //seek the right position of the event
  if(newOffset > 0){
    while(newOffset > (off_t)(lttdes->a_block_header->event_count - lttdes->which_event)){
      if(lttdes->which_block == lttdes->nbBlocks) return ENOENT; //end of file
      newOffset +=  - lttdes->a_block_header->event_count + lttdes->which_event;
      if((err=readBlock(lttdes, lttdes->which_block+1))!=0) return err;
    }        
  }else if(newOffset < 0){
    while(newOffset <= 0){
      if(lttdes->which_block == 1) return 0; //reach the beginning of the file
      if((err=readBlock(lttdes, lttdes->which_block-1))!=0)return err;
      newOffset +=  lttdes->a_block_header->event_count;	
      if(lttdes->which_block == 1) newOffset--;
    }
  }

  j = lttdes->which_event;
  for(i=j;i<j+newOffset;i++){
    err = trace_read(lttdes,&ev);
    if(err) return err;
  }  
  
  lttdes->which_event = i;
  if(lttdes->which_event == 0 || (lttdes->which_event == 1 && lttdes->which_block == 1))
    lttdes->current_event_time = lttdes->a_block_header->tsc;
  else  lttdes->current_event_time = ev.tsc;

  return 0;
}

/****************************************************************************
 *Function name
 *    readBlock       : read a block from the file
 *Input Params
 *    lttdes          : ltt file descriptor
 *    whichBlock      : the block which will be read
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 ****************************************************************************/
int readBlock(ltt_descriptor * lttdes, int whichBlock){
  off_t nbBytes;
  nbBytes = lseek(lttdes->fd, (off_t)((whichBlock-1)*lttdes->trace_header->buffer_size),SEEK_SET);
  if(nbBytes == -1) return EINVAL;
  
  if(readFile(lttdes->fd,lttdes->buffer,lttdes->trace_header->buffer_size,"Unable to read a block")) return EIO;
  lttdes->a_block_header = (block_header *) lttdes->buffer;
  lttdes->a_block_footer = (block_footer *)(lttdes->buffer + lttdes->trace_header->buffer_size - sizeof(block_footer)); 
  lttdes->which_block = whichBlock;
  lttdes->which_event = 0;
  lttdes->first_event_pos = lttdes->buffer + sizeof(block_header);
  if(lttdes->which_block == 1){
    lttdes->which_event++;
    lttdes->first_event_pos += sizeof(trace_header_event);
  }
  lttdes->cur_event_pos = lttdes->first_event_pos;
  lttdes->current_event_time = lttdes->a_block_header->tsc;

  getTSCPerUsec(lttdes);

  return 0;  
}

/****************************************************************************
 *Function name
 *    updateLttdes    : update the info of ltt descriptor
 *Input Params
 *    lttdes          : ltt file descriptor
 ****************************************************************************/
void updateLttdes(ltt_descriptor * lttdes){
  if(lttdes->which_block == 1)lttdes->which_event = 1;
  else lttdes->which_event = 0;
  lttdes->cur_event_pos = lttdes->first_event_pos;
  lttdes->current_event_time = lttdes->a_block_header->tsc;  
}

/****************************************************************************
 *Function name
 *    trace_seek_time : seek according to time
 *Input Params
 *    lttdes          : ltt file descriptor
 *    offset          : the file offset : time
 *    whence          : how to set offset, it can be SEEK_SET, SEEK_CUR, SEEK_END
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 *    ENOENT          : end of file
 ****************************************************************************/
int trace_seek_time(ltt_descriptor * lttdes, uint32_t offset, int whence){
  uint32_t curTime, seekTime;
  int blockNum, i,j, err;
  event_struct ev;
  void * tmpAddr;
  int whichEvent;
  uint32_t curEventTime;

  if(whence == SEEK_CUR && offset == 0)return 0;

  if(whence == SEEK_SET){
    if(offset < 0) return EINVAL;
    if(lttdes->which_block != 1){
      if((err=readBlock(lttdes, 1))!=0) return err;
    }else updateLttdes(lttdes);
    curTime = lttdes->a_block_header->tsc;
  }else if(whence == SEEK_END){
    if(offset > 0) return EINVAL;
    if(lttdes->which_block != lttdes->nbBlocks){
      if((err=readBlock(lttdes, lttdes->nbBlocks))!=0) return err;
    }else updateLttdes(lttdes);
    curTime = lttdes->a_block_footer->tsc;
  }else{
    curTime = lttdes->current_event_time;
  }
  
  seekTime = curTime + offset;

  //find the block which contains the time
  if(offset>0){
    if(whence == SEEK_SET)
      blockNum = -trace_get_time_block_position(lttdes, seekTime, 1, lttdes->nbBlocks);
    else if(whence == SEEK_CUR)
      blockNum = -trace_get_time_block_position(lttdes, seekTime, lttdes->which_block, lttdes->nbBlocks);
  }else if(offset<0){
    if(whence == SEEK_END)
      blockNum = -trace_get_time_block_position(lttdes, seekTime, 1, lttdes->nbBlocks);
    else if(whence == SEEK_CUR)
      blockNum = -trace_get_time_block_position(lttdes, seekTime, 1, lttdes->which_block);
  }else{
    if(whence == SEEK_SET) blockNum = 1;
    if(whence == SEEK_END) blockNum = lttdes->nbBlocks;
  }

  if(blockNum < 0) return -blockNum;
  
  //get the block
  if(lttdes->which_block != blockNum){
    if((err=readBlock(lttdes,blockNum))!=0) return err;
  }else updateLttdes(lttdes);

  if(lttdes->a_block_footer->tsc <= seekTime){
    if(blockNum == lttdes->nbBlocks) return ENOENT;  // end of file
    blockNum++;
  }

  //find the event whose time is just before the time
  if(lttdes->which_block != blockNum){
    if((err=readBlock(lttdes,blockNum))!=0) return err;
  }

  if(lttdes->current_event_time >= seekTime ) return 0; //find the right place
  
  //loop through the block to find the event
  j = lttdes->which_event;
  for(i=j;i<lttdes->a_block_header->event_count;i++){
    tmpAddr = lttdes->cur_event_pos;
    curEventTime = lttdes->current_event_time;
    whichEvent = lttdes->which_event;
    if((err = trace_read(lttdes,&ev))!=0) return err;
    if(ev.tsc >= seekTime){
      lttdes->cur_event_pos = tmpAddr;
      lttdes->current_event_time = curEventTime;
      lttdes->which_event = whichEvent;
      return 0;
    }
  }
  
  //if we reach here that means this block does not contain the time, go to the next block;
  if(blockNum == lttdes->nbBlocks) return ENOENT;  // end of file
  if((err=readBlock(lttdes,blockNum+1))!=0) return err;

  return 0;
}

/****************************************************************************
 *Function name
 *    trace_get_time_block_position : get the position of the block which 
 *                                    contains the seekTime
 *Input Params
 *    lttdes          : ltt file descriptor
 *    seekTime        : absolute time
 *    beginBlock      : the start block from which to begin the search
 *    endBlock        : the end block where to end the search
 *return value 
 *    negative number : the block number
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 *    ENOENT          : end of file
 ****************************************************************************/
int trace_get_time_block_position(ltt_descriptor * lttdes, uint32_t seekTime, int beginBlock, int endBlock){
  int err, middleBlock = (endBlock + beginBlock) / 2;
  
  if(beginBlock == endBlock)return -beginBlock;

  //get the start time of the block
  if(lttdes->which_block != middleBlock){
    if((err=readBlock(lttdes,middleBlock))!=0) return err;
  }else{
    updateLttdes(lttdes);
  }

  if(lttdes->a_block_header->tsc >= seekTime){
    if(middleBlock-beginBlock <= 1) return -beginBlock;    
    return trace_get_time_block_position(lttdes, seekTime,beginBlock, middleBlock-1);    
  }

  if(lttdes->a_block_footer->tsc >= seekTime) return -middleBlock;
  if(endBlock-middleBlock == 1) return -endBlock;
  return trace_get_time_block_position(lttdes, seekTime, middleBlock+1, endBlock);  
}

/****************************************************************************
 *Function names
 *   trace_get_char,   trace_get_uchar,   trace_get_enum,     trace_get_short
 *   trace_get_ushort, trace_get_integer, trace_get_uinteger, trace_get_long 
 *   trace_get_ulong,  trace_get_float,   trace_get_double,   trace_get_string
 *   the help functions are used to get data of special data type from event
 *   binary data
 *Input Params
 *   field       : field handle
 *   data        : event binary data
 *return value 
 *   corresponding data type
 ****************************************************************************/
int trace_get_char(field_handle * field, void * data){
  return (int)(*(char*)(data + field->offset));
}

int trace_get_uchar(field_handle * field, void * data){
  return (int)(*(unsigned char*)(data + field->offset));
}

unsigned long trace_get_enum(field_handle * field, void * data){
  if(field->size == 1)return (unsigned long)(*(uint8_t *)(data + field->offset));
  else if(field->size == 2)return (unsigned long)(*(uint16_t *)(data + field->offset));
  else if(field->size == 3)return (unsigned long)(*(uint32_t *)(data + field->offset));
  else if(field->size == 4)return *(unsigned long *)(data + field->offset);  
}

short int trace_get_short(field_handle * field, void * data){
  return *(short int*)(data + field->offset);
}

unsigned short int trace_get_ushort(field_handle * field, void * data){
  return *(unsigned short int*)(data + field->offset);
}

int trace_get_integer(field_handle * field, void * data){
  return *(int*)(data + field->offset);
}

unsigned int trace_get_uinteger(field_handle * field, void * data){
  return *(unsigned int*)(data + field->offset);
}

long trace_get_long(field_handle * field, void * data){
  return *(long*)(data + field->offset);
}

unsigned long trace_get_ulong(field_handle * field, void * data){
  return *(unsigned long*)(data + field->offset);
}

float trace_get_float(field_handle * field, void * data){
  return *(float*)(data + field->offset);
}

double trace_get_double(field_handle * field, void * data){
  return *(double*)(data + field->offset);
}

char * trace_get_string(field_handle * field, void * data){
  return allocAndCopy((char*)(data + field->offset));
}


//main and the following functions are just for the purpose of test, it will be deleted late

int main(char * argc, char ** argv){
  int i,j,k, err;
  facility_handle * fH;
  event_handle *evH;
  field_handle *fldH;
  int fd;
  block_header bh;
  block_footer bf, *ptr;
  trace_header_event thev;
  ltt_descriptor * lttdes;

  char buf[BUFFER_SIZE];

  fp = fopen("statistic","w");
  if(!fp) {
    printf("can not open test file\n");
    exit(1);
  }

  lttdes = trace_open_log(argv[1]);
  err = trace_seek(lttdes, 1, SEEK_SET);
  for(i=0;i<lttdes->a_block_header->event_count - 2;i++){
    err = trace_seek(lttdes, 1, SEEK_CUR);
  }
  ptr = (block_footer *)(lttdes->buffer + lttdes->trace_header->buffer_size - sizeof(block_footer));

/*
  err = trace_seek_time(lttdes, 11, SEEK_SET);
  err = trace_seek_time(lttdes, 1060000000, SEEK_CUR);
  while(1){
    err = trace_seek_time(lttdes, 100000000, SEEK_CUR);
    if(err) break;
  }
*/
}
