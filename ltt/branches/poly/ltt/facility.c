#include <stdlib.h> 
#include <string.h>
#include <stdio.h>

#include "LTTTypes.h"  
#include "parser.h"
#include <ltt/facility.h>

/* search for the (named) type in the table, if it does not exist
   create a new one */
ltt_type * lookup_named_type(ltt_facility *fac, type_descriptor * td);

/* construct directed acyclic graph for types, and tree for fields */
void constructTypeAndFields(ltt_facility * fac,type_descriptor * td, 
                            ltt_field * fld);

/* generate the facility according to the events belongin to it */
void generateFacility(ltt_facility * facility, char * pathname, 
                      ltt_checksum checksum, sequence * events);

/* functions to release the memory occupied by a facility */
void freeFacility(ltt_facility * facility);
void freeEventtype(ltt_eventtype * evType);
void freeAllNamedTypes(table * named_types);
void freeAllUnamedTypes(sequence * unnamed_types);
void freeAllFields(sequence * all_fields);
void freeLttType(ltt_type * type);
void freeLttField(ltt_field * fld);


/*****************************************************************************
 *Function name
 *    ltt_facility_open       : open a facility
 *Input params
 *    pathname                : the path name of the facility   
 *    c                       : checksum of the facility registered in kernal
 *Return value
 *    ltt_facility*           : return a ltt_facility
 ****************************************************************************/

ltt_facility * ltt_facility_open(char * pathname, ltt_checksum c)
{
  char *token;
  parse_file in;
  char buffer[BUFFER_SIZE];
  ltt_checksum checksum;
  event *ev;
  sequence  events;
  table namedTypes;
  sequence unnamedTypes;
  ltt_facility * aFacility = NULL;

  sequence_init(&events);
  table_init(&namedTypes);
  sequence_init(&unnamedTypes);

  in.buffer = buffer;
  in.lineno = 0;
  in.error = error_callback;
  in.name = appendString(pathname,".event");

  in.fp = fopen(in.name, "r");
  if(!in.fp )g_error("cannot open input file: %s\n", in.name);
 
  while(1){
    token = getToken(&in);
    if(in.type == ENDFILE) break;
    
    if(strcmp("event",token) == 0) {
      ev = g_new(event,1);
      sequence_push(&events,ev);
      parseEvent(&in,ev, &unnamedTypes, &namedTypes);
    }
    else if(strcmp("type",token) == 0) {
      parseTypeDefinition(&in, &unnamedTypes, &namedTypes);
    }
    else g_error("event or type token expected\n");
  }
  
  fclose(in.fp);

  checkNamedTypesImplemented(&namedTypes);

  generateChecksum(pathname, &checksum, &events);

  //yxx disable during the test
  aFacility = g_new(ltt_facility,1);    
  generateFacility(aFacility, pathname, checksum, &events);
/*
  if(checksum == c){
    aFacility = g_new(ltt_facility,1);    
    generateFacility(aFacility, pathname, checksum, &events);
  }else{
    g_error("local facility is different from the one registered in the kernel");
  }
*/

  free(in.name);
  freeEvents(&events);
  sequence_dispose(&events);
  freeNamedType(&namedTypes);
  table_dispose(&namedTypes);
  freeTypes(&unnamedTypes);
  sequence_dispose(&unnamedTypes);

  return aFacility;
}


/*****************************************************************************
 *Function name
 *    generateFacility    : generate facility, internal function
 *Input params 
 *    facility            : facilty structure
 *    facName             : facility name
 *    checksum            : checksum of the facility          
 *    events              : sequence of events belonging to the facility
 ****************************************************************************/

void generateFacility(ltt_facility * facility, char * pathname, 
                      ltt_checksum checksum, sequence * events)
{
  char * facilityName;
  int i;
  ltt_eventtype * evType;
  ltt_field * field;
  ltt_type * type;

  //get the facility name (strip any leading directory) 
  facilityName = strrchr(pathname,'/');
  if(facilityName) facilityName++;    
  else facilityName = pathname;
  
  facility->name = g_strdup(facilityName);
  facility->event_number = events->position;
  facility->checksum = checksum;
  facility->usage_count = 0;
  
  //initialize inner structures
  facility->events = g_new(ltt_eventtype*,facility->event_number); 
  sequence_init(&(facility->all_fields));
  sequence_init(&(facility->all_unnamed_types));
  table_init(&(facility->all_named_types));

  //for each event, construct field tree and type graph
  for(i=0;i<events->position;i++){
    evType = g_new(ltt_eventtype,1);
    facility->events[i] = evType;

    evType->name = g_strdup(((event*)(events->array[i]))->name);
    evType->description=g_strdup(((event*)(events->array[i]))->description);
    
    field = g_new(ltt_field, 1);
    sequence_push(&(facility->all_fields), field);
    evType->root_field = field;
    evType->facility = facility;
    evType->index = i;

    field->field_pos = 0;
    type = lookup_named_type(facility,((event*)(events->array[i]))->type);
    field->field_type = type;
    field->offset_root = 0;
    field->fixed_root = 1;
    field->offset_parent = 0;
    field->fixed_parent = 1;
    //    field->base_address = NULL;
    field->field_size  = 0;
    field->field_fixed = -1;
    field->parent = NULL;
    field->child = NULL;
    field->current_element = 0;

    //construct field tree and type graph
    constructTypeAndFields(facility,((event*)(events->array[i]))->type,field);
  }  
}


/*****************************************************************************
 *Function name
 *    constructTypeAndFields : construct field tree and type graph,
 *                             internal recursion function
 *Input params 
 *    fac                    : facility struct
 *    td                     : type descriptor
 *    root_field             : root field of the event
 ****************************************************************************/

void constructTypeAndFields(ltt_facility * fac,type_descriptor * td, 
                            ltt_field * fld)
{
  int i;
  type_descriptor * tmpTd;

  //  if(td->type == LTT_STRING || td->type == LTT_SEQUENCE)
  //    fld->field_size = 0;
  //  else fld->field_size = -1;

  if(td->type == LTT_ENUM){
    fld->field_type->element_number = td->labels.position;
    fld->field_type->enum_strings = g_new(char*,td->labels.position);
    for(i=0;i<td->labels.position;i++){
      fld->field_type->enum_strings[i] 
                          = g_strdup(((char*)(td->labels.array[i])));
    }
  }else if(td->type == LTT_ARRAY || td->type == LTT_SEQUENCE){
    if(td->type == LTT_ARRAY)
      fld->field_type->element_number = (unsigned)td->size;
    fld->field_type->element_type = g_new(ltt_type*,1);
    tmpTd = td->nested_type;
    fld->field_type->element_type[0] = lookup_named_type(fac, tmpTd);
    fld->child = g_new(ltt_field*, 1);
    fld->child[0] = g_new(ltt_field, 1);
    sequence_push(&(fac->all_fields), fld->child[0]);
    
    fld->child[0]->field_pos = 0;
    fld->child[0]->field_type = fld->field_type->element_type[0];
    fld->child[0]->offset_root = fld->offset_root;
    fld->child[0]->fixed_root = fld->fixed_root;
    fld->child[0]->offset_parent = 0;
    fld->child[0]->fixed_parent = 1;
    //    fld->child[0]->base_address = NULL;
    fld->child[0]->field_size  = 0;
    fld->child[0]->field_fixed = -1;
    fld->child[0]->parent = fld;
    fld->child[0]->child = NULL;
    fld->child[0]->current_element = 0;
    constructTypeAndFields(fac, tmpTd, fld->child[0]);
  }else if(td->type == LTT_STRUCT){
    fld->field_type->element_number = td->fields.position;
    fld->field_type->element_type = g_new(ltt_type*, td->fields.position);
    fld->child = g_new(ltt_field*, td->fields.position);      
    for(i=0;i<td->fields.position;i++){
      tmpTd = ((field*)(td->fields.array[i]))->type;
      fld->field_type->element_type[i] = lookup_named_type(fac, tmpTd);
      fld->child[i] = g_new(ltt_field,1); 
      sequence_push(&(fac->all_fields), fld->child[i]);

      fld->child[i]->field_pos = i;
      fld->child[i]->field_type = fld->field_type->element_type[i]; 
      fld->child[i]->field_type->element_name 
                      = g_strdup(((field*)(td->fields.array[i]))->name);
      fld->child[i]->offset_root = -1;
      fld->child[i]->fixed_root = -1;
      fld->child[i]->offset_parent = -1;
      fld->child[i]->fixed_parent = -1;
      //      fld->child[i]->base_address = NULL;
      fld->child[i]->field_size  = 0;
      fld->child[i]->field_fixed = -1;
      fld->child[i]->parent = fld;
      fld->child[i]->child = NULL;
      fld->child[i]->current_element = 0;
      constructTypeAndFields(fac, tmpTd, fld->child[i]);
    }    
  }
}


/*****************************************************************************
 *Function name
 *    lookup_named_type: search named type in the table
 *                       internal function
 *Input params 
 *    fac              : facility struct
 *    td               : type descriptor
 *Return value    
 *                     : either find the named type, or create a new ltt_type
 ****************************************************************************/

ltt_type * lookup_named_type(ltt_facility *fac, type_descriptor * td)
{
  ltt_type * lttType = NULL;
  int i;
  char * name;
  if(td->type_name){
    for(i=0;i<fac->all_named_types.keys.position;i++){
      name = (char *)(fac->all_named_types.keys.array[i]);
      if(strcmp(name, td->type_name)==0){
	lttType = (ltt_type*)(fac->all_named_types.values.array[i]);
	break;
      }
    }
  }
  
  if(!lttType){
    lttType = g_new(ltt_type,1);
    lttType->type_class = td->type;
    if(td->fmt) lttType->fmt = g_strdup(td->fmt);
    else lttType->fmt = NULL;
    lttType->size = td->size;
    lttType->enum_strings = NULL;
    lttType->element_type = NULL;
    lttType->element_number = 0;
    if(td->type_name){
      name = g_strdup(td->type_name);
      table_insert(&(fac->all_named_types),name,lttType);
      lttType->element_name = name;
    }
    else{
      sequence_push(&(fac->all_unnamed_types), lttType);
      lttType->element_name = NULL;
    }
  }

  return lttType;
}


/*****************************************************************************
 *Function name
 *    ltt_facility_close      : close a facility, decrease its usage count,
 *                              if usage count = 0, release the memory
 *Input params
 *    f                       : facility that will be closed
 *Return value
 *    int                     : usage count ?? status
 ****************************************************************************/

int ltt_facility_close(ltt_facility *f)
{
  //  f->usage_count--;
  if(f->usage_count > 0) return f->usage_count;
  
  //release the memory it occupied
  freeFacility(f);

  return 0;
}

/*****************************************************************************
 * Functions to release the memory occupied by the facility
 ****************************************************************************/

void freeFacility(ltt_facility * fac)
{
  int i;
  g_free(fac->name);  //free facility name

  //free event types
  for(i=0;i<fac->event_number;i++){
    freeEventtype(fac->events[i]);
  }

  //free all named types
  freeAllNamedTypes(&(fac->all_named_types));

  //free all unnamed types
  freeAllUnamedTypes(&(fac->all_unnamed_types));

  //free all fields
  freeAllFields(&(fac->all_fields));

  //free the facility itself
  g_free(fac);
}

void freeEventtype(ltt_eventtype * evType)
{
  g_free(evType->name);
  if(evType->description)
    g_free(evType->description); 
  g_free(evType);
}

void freeAllNamedTypes(table * named_types)
{
  int i;
  for(i=0;i<named_types->keys.position;i++){
    //free the name of the type
    g_free((char*)(named_types->keys.array[i]));
    
    //free type
    freeLttType((ltt_type*)(named_types->values.array[i]));
  }
  table_dispose(named_types);
}

void freeAllUnamedTypes(sequence * unnamed_types)
{
  int i;
  for(i=0;i<unnamed_types->position;i++){
    freeLttType((ltt_type*)(unnamed_types->array[i]));
  }
  sequence_dispose(unnamed_types);
}

void freeAllFields(sequence * all_fields)
{
  int i;
  for(i=0;i<all_fields->position;i++){
    freeLttField((ltt_field*)(all_fields->array[i]));
  }
  sequence_dispose(all_fields);
}

void freeLttType(ltt_type * type)
{
  if(type->element_name)
    g_free(type->element_name);
  if(type->fmt)
    g_free(type->fmt);
  if(type->enum_strings)
    g_free(type->enum_strings);
  if(type->element_type)
    g_free(type->element_type);
  g_free(type);
}

void freeLttField(ltt_field * fld)
{  
  if(fld->child)
    g_free(fld->child);
  g_free(fld);
}

/*****************************************************************************
 *Function name
 *    ltt_facility_name       : obtain the facility's name
 *Input params
 *    f                       : the facility that will be closed
 *Return value
 *    char *                  : the facility's name
 ****************************************************************************/

char *ltt_facility_name(ltt_facility *f)
{
  return f->name;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_checksum   : obtain the facility's checksum
 *Input params
 *    f                       : the facility that will be closed
 *Return value
 *    ltt_checksum            : the checksum of the facility 
 ****************************************************************************/

ltt_checksum ltt_facility_checksum(ltt_facility *f)
{
  return f->checksum;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_eventtype_number: obtain the number of the event types 
 *Input params
 *    f                            : the facility that will be closed
 *Return value
 *    unsigned                     : the number of the event types 
 ****************************************************************************/

unsigned ltt_facility_eventtype_number(ltt_facility *f)
{
  return (unsigned)(f->event_number);
}

/*****************************************************************************
 *Function name
 *    ltt_facility_eventtype_get: obtain the event type according to event id
 *                                from 0 to event_number - 1
 *Input params
 *    f                         : the facility that will be closed
 *Return value
 *    ltt_eventtype *           : the event type required  
 ****************************************************************************/

ltt_eventtype *ltt_facility_eventtype_get(ltt_facility *f, unsigned i)
{
  return f->events[i];
}

/*****************************************************************************
 *Function name
 *    ltt_facility_eventtype_get_by_name
 *                     : obtain the event type according to event name
 *                       event name is unique in the facility
 *Input params
 *    f                : the facility that will be closed
 *    name             : the name of the event
 *Return value
 *    ltt_eventtype *  : the event type required  
 ****************************************************************************/

ltt_eventtype *ltt_facility_eventtype_get_by_name(ltt_facility *f, char *name)
{
  int i;
  ltt_eventtype * ev;
  for(i=0;i<f->event_number;i++){
    ev = f->events[i];
    if(strcmp(ev->name, name) == 0)break;      
  }

  if(i==f->event_number) return NULL;
  else return ev;
}

