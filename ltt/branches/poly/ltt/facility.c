#include <stdlib.h> 
#include <string.h>
#include <stdio.h>

#include <ltt/LTTTypes.h>  
#include "parser.h"
#include <ltt/facility.h>

/* search for the (named) type in the table, if it does not exist
   create a new one */
LttType * lookup_named_type(LttFacility *fac, type_descriptor * td);

/* construct directed acyclic graph for types, and tree for fields */
void constructTypeAndFields(LttFacility * fac,type_descriptor * td, 
                            LttField * fld);

/* generate the facility according to the events belongin to it */
void generateFacility(LttFacility * f, facility  * fac, 
                      LttChecksum checksum);

/* functions to release the memory occupied by a facility */
void freeFacility(LttFacility * facility);
void freeEventtype(LttEventType * evType);
void freeAllNamedTypes(table * named_types);
void freeAllUnamedTypes(sequence * unnamed_types);
void freeAllFields(sequence * all_fields);
void freeLttType(LttType * type);
void freeLttField(LttField * fld);


/*****************************************************************************
 *Function name
 *    ltt_facility_open       : open facilities
 *Input params
 *    t                       : the trace containing the facilities
 *    pathname                : the path name of the facility   
 ****************************************************************************/

void ltt_facility_open(LttTrace * t, char * pathname)
{
  char *token;
  parse_file in;
  char buffer[BUFFER_SIZE];
  facility * fac;
  LttFacility * f;
  LttChecksum checksum;

  in.buffer = buffer;
  in.lineno = 0;
  in.error = error_callback;
  in.name = pathname;

  in.fp = fopen(in.name, "r");
  if(!in.fp ) in.error(&in,"cannot open input file");

  while(1){
    token = getToken(&in);
    if(in.type == ENDFILE) break;
    
    if(strcmp(token, "<")) in.error(&in,"not a facility file");
    token = getName(&in);
    
    if(strcmp("facility",token) == 0) {
      fac = g_new(facility, 1);
      fac->name = NULL;
      fac->description = NULL;
      sequence_init(&(fac->events));
      table_init(&(fac->named_types));
      sequence_init(&(fac->unnamed_types));
      
      parseFacility(&in, fac);

      //check if any namedType is not defined
      checkNamedTypesImplemented(&fac->named_types);
    
      generateChecksum(fac->name, &checksum, &fac->events);

      f = g_new(LttFacility,1);    
      generateFacility(f, fac, checksum);

      t->facility_number++;
      g_ptr_array_add(t->facilities,f);

      free(fac->name);
      free(fac->description);
      freeEvents(&fac->events);
      sequence_dispose(&fac->events);
      freeNamedType(&fac->named_types);
      table_dispose(&fac->named_types);
      freeTypes(&fac->unnamed_types);
      sequence_dispose(&fac->unnamed_types);      
      g_free(fac);
    }
    else in.error(&in,"facility token was expected");
  }
  fclose(in.fp);
}


/*****************************************************************************
 *Function name
 *    generateFacility    : generate facility, internal function
 *Input params 
 *    facility            : LttFacilty structure
 *    fac                 : facility structure
 *    checksum            : checksum of the facility          
 ****************************************************************************/

void generateFacility(LttFacility *f, facility *fac,LttChecksum checksum)
{
  char * facilityName = fac->name;
  sequence * events = &fac->events;
  int i;
  LttEventType * evType;
  LttField * field;
  LttType * type;
  
  f->name = g_strdup(facilityName);
  f->event_number = events->position;
  f->checksum = checksum;
  
  //initialize inner structures
  f->events = g_new(LttEventType*,f->event_number); 
  sequence_init(&(f->all_fields));
  sequence_init(&(f->all_unnamed_types));
  table_init(&(f->all_named_types));

  //for each event, construct field tree and type graph
  for(i=0;i<events->position;i++){
    evType = g_new(LttEventType,1);
    f->events[i] = evType;

    evType->name = g_strdup(((event*)(events->array[i]))->name);
    evType->description=g_strdup(((event*)(events->array[i]))->description);
    
    field = g_new(LttField, 1);
    sequence_push(&(f->all_fields), field);
    evType->root_field = field;
    evType->facility = f;
    evType->index = i;

    if(((event*)(events->array[i]))->type != NULL){
      field->field_pos = 0;
      type = lookup_named_type(f,((event*)(events->array[i]))->type);
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
      constructTypeAndFields(f,((event*)(events->array[i]))->type,field);
    }else{
      evType->root_field = NULL;
      sequence_pop(&(f->all_fields));
      g_free(field);
    }
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

void constructTypeAndFields(LttFacility * fac,type_descriptor * td, 
                            LttField * fld)
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
    fld->field_type->element_type = g_new(LttType*,1);
    tmpTd = td->nested_type;
    fld->field_type->element_type[0] = lookup_named_type(fac, tmpTd);
    fld->child = g_new(LttField*, 1);
    fld->child[0] = g_new(LttField, 1);
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
    fld->field_type->element_type = g_new(LttType*, td->fields.position);
    fld->child = g_new(LttField*, td->fields.position);      
    for(i=0;i<td->fields.position;i++){
      tmpTd = ((field*)(td->fields.array[i]))->type;
      fld->field_type->element_type[i] = lookup_named_type(fac, tmpTd);
      fld->child[i] = g_new(LttField,1); 
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
 *                     : either find the named type, or create a new LttType
 ****************************************************************************/

LttType * lookup_named_type(LttFacility *fac, type_descriptor * td)
{
  LttType * lttType = NULL;
  int i;
  char * name;
  if(td->type_name){
    for(i=0;i<fac->all_named_types.keys.position;i++){
      name = (char *)(fac->all_named_types.keys.array[i]);
      if(strcmp(name, td->type_name)==0){
	lttType = (LttType*)(fac->all_named_types.values.array[i]);
	break;
      }
    }
  }
  
  if(!lttType){
    lttType = g_new(LttType,1);
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

int ltt_facility_close(LttFacility *f)
{
  //release the memory it occupied
  freeFacility(f);

  return 0;
}

/*****************************************************************************
 * Functions to release the memory occupied by the facility
 ****************************************************************************/

void freeFacility(LttFacility * fac)
{
  int i;
  g_free(fac->name);  //free facility name

  //free event types
  for(i=0;i<fac->event_number;i++){
    freeEventtype(fac->events[i]);
  }
  g_free(fac->events);

  //free all named types
  freeAllNamedTypes(&(fac->all_named_types));

  //free all unnamed types
  freeAllUnamedTypes(&(fac->all_unnamed_types));

  //free all fields
  freeAllFields(&(fac->all_fields));

  //free the facility itself
  g_free(fac);
}

void freeEventtype(LttEventType * evType)
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
    freeLttType((LttType*)(named_types->values.array[i]));
  }
  table_dispose(named_types);
}

void freeAllUnamedTypes(sequence * unnamed_types)
{
  int i;
  for(i=0;i<unnamed_types->position;i++){
    freeLttType((LttType*)(unnamed_types->array[i]));
  }
  sequence_dispose(unnamed_types);
}

void freeAllFields(sequence * all_fields)
{
  int i;
  for(i=0;i<all_fields->position;i++){
    freeLttField((LttField*)(all_fields->array[i]));
  }
  sequence_dispose(all_fields);
}

//only free current type, not child types
void freeLttType(LttType * type)
{
  int i;
  if(type->element_name)
    g_free(type->element_name);
  if(type->fmt)
    g_free(type->fmt);
  if(type->enum_strings){
    for(i=0;i<type->element_number;i++)
      g_free(type->enum_strings[i]);
    g_free(type->enum_strings);
  }

  if(type->element_type){
    g_free(type->element_type);
  }
  g_free(type);
}

//only free the current field, not child fields
void freeLttField(LttField * fld)
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

char *ltt_facility_name(LttFacility *f)
{
  return f->name;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_checksum   : obtain the facility's checksum
 *Input params
 *    f                       : the facility that will be closed
 *Return value
 *    LttChecksum            : the checksum of the facility 
 ****************************************************************************/

LttChecksum ltt_facility_checksum(LttFacility *f)
{
  return f->checksum;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_base_id    : obtain the facility base id
 *Input params
 *    f                       : the facility
 *Return value
 *                            : the base id of the facility
 ****************************************************************************/

unsigned ltt_facility_base_id(LttFacility *f)
{
  return f->base_id;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_eventtype_number: obtain the number of the event types 
 *Input params
 *    f                            : the facility that will be closed
 *Return value
 *    unsigned                     : the number of the event types 
 ****************************************************************************/

unsigned ltt_facility_eventtype_number(LttFacility *f)
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
 *    LttEventType *           : the event type required  
 ****************************************************************************/

LttEventType *ltt_facility_eventtype_get(LttFacility *f, unsigned i)
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
 *    LttEventType *  : the event type required  
 ****************************************************************************/

LttEventType *ltt_facility_eventtype_get_by_name(LttFacility *f, char *name)
{
  int i;
  LttEventType * ev;
  for(i=0;i<f->event_number;i++){
    ev = f->events[i];
    if(strcmp(ev->name, name) == 0)break;      
  }

  if(i==f->event_number) return NULL;
  else return ev;
}

