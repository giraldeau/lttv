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

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/facility.h>

/* search for the (named) type in the table, if it does not exist
   create a new one */
LttType * lookup_named_type(LttFacility *fac, type_descriptor * td);

/* construct directed acyclic graph for types, and tree for fields */
void constructTypeAndFields(LttFacility * fac,type_descriptor * td, 
                            LttField * fld);

/* generate the facility according to the events belongin to it */
void generateFacility(LttFacility * f, facility_t  * fac, 
                      LttChecksum checksum);

/* functions to release the memory occupied by a facility */
void freeFacility(LttFacility * facility);
void freeEventtype(LttEventType * evType);
void freeLttType(LttType ** type);
void freeLttField(LttField * fld);
void freeLttNamedType(LttType * type);


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
  facility_t * fac;
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
      fac = g_new(facility_t, 1);
      fac->name = NULL;
      fac->description = NULL;
      sequence_init(&(fac->events));
      table_init(&(fac->named_types));
      sequence_init(&(fac->unnamed_types));
      
      parseFacility(&in, fac);

      //check if any namedType is not defined
      g_assert(checkNamedTypesImplemented(&fac->named_types) == 0);
    
      g_assert(generateChecksum(fac->name, &checksum, &fac->events) == 0);

      f = g_new(LttFacility,1);    
      f->base_id = 0;
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

void generateFacility(LttFacility *f, facility_t *fac,LttChecksum checksum)
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
  f->named_types_number = fac->named_types.keys.position;
  f->named_types = g_new(LttType*, fac->named_types.keys.position);
  for(i=0;i<fac->named_types.keys.position;i++) f->named_types[i] = NULL;

  //for each event, construct field tree and type graph
  for(i=0;i<events->position;i++){
    evType = g_new(LttEventType,1);
    f->events[i] = evType;

    evType->name = g_strdup(((event_t*)(events->array[i]))->name);
    evType->description=g_strdup(((event_t*)(events->array[i]))->description);
    
    field = g_new(LttField, 1);
    evType->root_field = field;
    evType->facility = f;
    evType->index = i;

    if(((event_t*)(events->array[i]))->type != NULL){
      field->field_pos = 0;
      type = lookup_named_type(f,((event_t*)(events->array[i]))->type);
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
      constructTypeAndFields(f,((event_t*)(events->array[i]))->type,field);
    }else{
      evType->root_field = NULL;
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
  int i, flag;
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

    if(fld->field_type->element_type == NULL){
      fld->field_type->element_type = g_new(LttType*, td->fields.position);
      flag = 1;
    }else{
      flag = 0;
    }

    fld->child = g_new(LttField*, td->fields.position);      
    for(i=0;i<td->fields.position;i++){
      tmpTd = ((type_fields*)(td->fields.array[i]))->type;

      if(flag)
	fld->field_type->element_type[i] = lookup_named_type(fac, tmpTd);
      fld->child[i] = g_new(LttField,1); 

      fld->child[i]->field_pos = i;
      fld->child[i]->field_type = fld->field_type->element_type[i]; 

      if(flag){
	fld->child[i]->field_type->element_name 
	  = g_strdup(((type_fields*)(td->fields.array[i]))->name);
      }

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
  unsigned int i=0;
  char * name;

  if(td->type_name){
    for(i=0;i<fac->named_types_number; i++){
      if(fac->named_types[i] == NULL) break;
      name = fac->named_types[i]->type_name;
      if(strcmp(name, td->type_name)==0){
	      lttType = fac->named_types[i];	
	//	if(lttType->element_name) g_free(lttType->element_name);
	//	lttType->element_name = NULL;
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
    lttType->element_name = NULL;
    if(td->type_name){
      lttType->type_name = g_strdup(td->type_name);
      fac->named_types[i] = lttType; /* i is initialized, checked. */
    }
    else{
      lttType->type_name = NULL;
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
  unsigned int i;
  g_free(fac->name);  //free facility name

  //free event types
  for(i=0;i<fac->event_number;i++){
    freeEventtype(fac->events[i]);
  }
  g_free(fac->events);

  //free all named types
  for(i=0;i<fac->named_types_number;i++){
    freeLttNamedType(fac->named_types[i]);
    fac->named_types[i] = NULL;
  }
  g_free(fac->named_types);

  //free the facility itself
  g_free(fac);
}

void freeEventtype(LttEventType * evType)
{
  LttType * root_type;
  g_free(evType->name);
  if(evType->description)
    g_free(evType->description); 
  if(evType->root_field){    
    root_type = evType->root_field->field_type;
    freeLttField(evType->root_field);
    freeLttType(&root_type);
  }

  g_free(evType);
}

void freeLttNamedType(LttType * type)
{
  g_free(type->type_name);
  type->type_name = NULL;
  freeLttType(&type);
}

void freeLttType(LttType ** type)
{
  unsigned int i;
  if(*type == NULL) return;
  if((*type)->type_name){
    return; //this is a named type
  }
  if((*type)->element_name)
    g_free((*type)->element_name);
  if((*type)->fmt)
    g_free((*type)->fmt);
  if((*type)->enum_strings){
    for(i=0;i<(*type)->element_number;i++)
      g_free((*type)->enum_strings[i]);
    g_free((*type)->enum_strings);
  }

  if((*type)->element_type){
    for(i=0;i<(*type)->element_number;i++)
      freeLttType(&((*type)->element_type[i]));   
    g_free((*type)->element_type);
  }
  g_free(*type);
  *type = NULL;
}

void freeLttField(LttField * fld)
{ 
  int i;
  int size = 0;
  
  if(fld->field_type){
    if(fld->field_type->type_class == LTT_ARRAY ||
       fld->field_type->type_class == LTT_SEQUENCE){
      size = 1;
    }else if(fld->field_type->type_class == LTT_STRUCT){
      size = fld->field_type->element_number;
    }
  }

  if(fld->child){
    for(i=0; i<size; i++){
      if(fld->child[i])freeLttField(fld->child[i]);
    }
    g_free(fld->child);
  }
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
  return (f->event_number);
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
 *    f                : the facility
 *    name             : the name of the event
 *Return value
 *    LttEventType *  : the event type required  
 ****************************************************************************/

LttEventType *ltt_facility_eventtype_get_by_name(LttFacility *f, char *name)
{
  unsigned int i;
  LttEventType * ev = NULL;
  
  for(i=0;i<f->event_number;i++){
    LttEventType *iter_ev = f->events[i];
    if(strcmp(iter_ev->name, name) == 0) {
      ev = iter_ev;
      break;
    }
  }
  return ev;
}

