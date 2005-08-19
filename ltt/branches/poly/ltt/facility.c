/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
 *               2005 Mathieu Desnoyers
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/facility.h>

#ifndef g_open
#define g_open open
#endif

#define g_close close

/* search for the (named) type in the table, if it does not exist
   create a new one */
LttType * lookup_named_type(LttFacility *fac, type_descriptor * td);

/* construct directed acyclic graph for types, and tree for fields */
void construct_types_and_fields(LttFacility * fac,type_descriptor * td, 
                            LttField * fld);

/* generate the facility according to the events belongin to it */
void generateFacility(LttFacility * f, facility_t  * fac, 
                      guint32 checksum);

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
 *
 *returns 0 on success, 1 on error.
 ****************************************************************************/

int ltt_facility_open(LttFacility *f, LttTrace * t, gchar * pathname)
{
  gchar *token;
  parse_file in;
  gsize length;
  facility_t * fac;
  guint32 checksum;
  GError * error = NULL;
  gchar buffer[BUFFER_SIZE];

  in.buffer = &(buffer[0]);
  in.lineno = 0;
  in.error = error_callback;
  in.name = pathname;

  in.fd = g_open(in.name, O_RDONLY, 0);
  if(in.fd < 0 ) {
    g_warning("cannot open facility description file %s",
        in.name);
    return 1;
  }

  in.channel = g_io_channel_unix_new(in.fd);
  in.pos = 0;

  while(1){
    token = getToken(&in);
    if(in.type == ENDFILE) break;
    
    if(g_ascii_strcasecmp(token, "<")) in.error(&in,"not a facility file");
    token = getName(&in);

    if(g_ascii_strcasecmp("facility",token) == 0) {
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

      generateFacility(f, fac, checksum);

      g_free(fac->name);
      g_free(fac->description);
      freeEvents(&fac->events);
      sequence_dispose(&fac->events);
      freeNamedType(&fac->named_types);
      table_dispose(&fac->named_types);
      freeTypes(&fac->unnamed_types);
      sequence_dispose(&fac->unnamed_types);      
      g_free(fac);
    }
    else {
      g_warning("facility token was expected in file %s", in.name);
      goto parse_error;
    }
  }

 parse_error:
  g_io_channel_shutdown(in.channel, FALSE, &error); /* No flush */
  if(error != NULL) {
    g_warning("Can not close file: \n%s\n", error->message);
    g_error_free(error);
  }

  g_close(in.fd);
}


/*****************************************************************************
 *Function name
 *    generateFacility    : generate facility, internal function
 *Input params 
 *    facility            : LttFacilty structure
 *    fac                 : facility structure
 *    checksum            : checksum of the facility          
 ****************************************************************************/

void generateFacility(LttFacility *f, facility_t *fac, guint32 checksum)
{
  char * facilityName = fac->name;
  sequence * events = &fac->events;
  int i;
  //LttEventType * evType;
  LttEventType * event_type;
  LttField * field;
  LttType * type;
  
  g_assert(f->name == g_quark_from_string(facilityName));
  g_assert(f->checksum == checksum);

  //f->event_number = events->position;
  
  //initialize inner structures
  f->events = g_array_sized_new (FALSE, TRUE, sizeof(LttEventType),
      events->position);
  //f->events = g_new(LttEventType*,f->event_number);
  f->events = g_array_set_size(f->events, events->position);

  g_datalist_init(&f->events_by_name);
  g_datalist_init(&f->named_types);
  
  //f->named_types_number = fac->named_types.keys.position;
  //f->named_types = g_array_sized_new (FALSE, TRUE, sizeof(LttType),
  //    fac->named_types.keys.position);
  //f->named_types = g_new(LttType*, fac->named_types.keys.position);
  //f->named_types = g_array_set_size(f->named_types, 
  //    fac->named_types.keys.position);

  //for each event, construct field tree and type graph
  for(i=0;i<events->position;i++){
    event_type = &g_array_index(f->events, LttEventType, i);
    //evType = g_new(LttEventType,1);
    //f->events[i] = evType;

    event_type->name = 
      g_quark_from_string(((event_t*)(events->array[i]))->name);
    
    g_datalist_id_set_data(&f->events_by_name, event_type->name,
        event_type);
    
    event_type->description =
      g_strdup(((event_t*)(events->array[i]))->description);
    
    field = g_new(LttField, 1);
    event_type->root_field = field;
    event_type->facility = f;
    event_type->index = i;

    if(((event_t*)(events->array[i]))->type != NULL){
 //     field->field_pos = 0;
      type = lookup_named_type(f,((event_t*)(events->array[i]))->type);
      field->field_type = type;
      field->offset_root = 0;
      field->fixed_root = FIELD_UNKNOWN;
      field->offset_parent = 0;
      field->fixed_parent = FIELD_UNKNOWN;
      //    field->base_address = NULL;
      field->field_size  = 0;
      field->fixed_size = FIELD_UNKNOWN;
      field->parent = NULL;
      field->child = NULL;
      field->current_element = 0;

      //construct field tree and type graph
      construct_types_and_fields(f,((event_t*)(events->array[i]))->type,field);
    }else{
      event_type->root_field = NULL;
      g_free(field);
    }
  }  
}


/*****************************************************************************
 *Function name
 *    construct_types_and_fields : construct field tree and type graph,
 *                             internal recursion function
 *Input params 
 *    fac                    : facility struct
 *    td                     : type descriptor
 *    root_field             : root field of the event
 ****************************************************************************/


void construct_types_and_fields(LttFacility * fac, type_descriptor * td, 
                            LttField * fld)
{
  int i, flag;
  type_descriptor * tmpTd;

  switch(td->type) {
  case LTT_ENUM:
    fld->field_type->element_number = td->labels.position;
    fld->field_type->enum_strings = g_new(GQuark,td->labels.position);
    for(i=0;i<td->labels.position;i++){
      fld->field_type->enum_strings[i] 
                     = g_quark_from_string(((char*)(td->labels.array[i])));
    }
    break;
  case LTT_ARRAY:
    fld->field_type->element_number = (unsigned)td->size;
  case LTT_SEQUENCE:
    fld->field_type->element_type = g_new(LttType*,1);
    tmpTd = td->nested_type;
    fld->field_type->element_type[0] = lookup_named_type(fac, tmpTd);
    fld->child = g_new(LttField*, 1);
    fld->child[0] = g_new(LttField, 1);
    
    fld->child[0]->field_type = fld->field_type->element_type[0];
    fld->child[0]->offset_root = 0;
    fld->child[0]->fixed_root = FIELD_UNKNOWN;
    fld->child[0]->offset_parent = 0;
    fld->child[0]->fixed_parent = FIELD_UNKNOWN;
    fld->child[0]->field_size  = 0;
    fld->child[0]->fixed_size = FIELD_UNKNOWN;
    fld->child[0]->parent = fld;
    fld->child[0]->child = NULL;
    fld->child[0]->current_element = 0;
    construct_types_and_fields(fac, tmpTd, fld->child[0]);
    break;
  case LTT_STRUCT:
  case LTT_UNION:
    fld->field_type->element_number = td->fields.position;

    g_assert(fld->field_type->element_type == NULL);
    fld->field_type->element_type = g_new(LttType*, td->fields.position);

    fld->child = g_new(LttField*, td->fields.position);      
    for(i=0;i<td->fields.position;i++){
      tmpTd = ((type_fields*)(td->fields.array[i]))->type;

    	fld->field_type->element_type[i] = lookup_named_type(fac, tmpTd);
      fld->child[i] = g_new(LttField,1); 

 //     fld->child[i]->field_pos = i;
      fld->child[i]->field_type = fld->field_type->element_type[i]; 

      fld->child[i]->field_type->element_name 
	          = g_quark_from_string(((type_fields*)(td->fields.array[i]))->name);

      fld->child[i]->offset_root = 0;
      fld->child[i]->fixed_root = FIELD_UNKNOWN;
      fld->child[i]->offset_parent = 0;
      fld->child[i]->fixed_parent = FIELD_UNKNOWN;
      fld->child[i]->field_size  = 0;
      fld->child[i]->fixed_size = FIELD_UNKNOWN;
      fld->child[i]->parent = fld;
      fld->child[i]->child = NULL;
      fld->child[i]->current_element = 0;
      construct_types_and_fields(fac, tmpTd, fld->child[i]);
    }    

    break;
  default:
    g_error("construct_types_and_fields : unknown type");
  }


}



#if 0
void construct_types_and_fields(LttFacility * fac, type_descriptor * td, 
                            LttField * fld)
{
  int i, flag;
  type_descriptor * tmpTd;

  //  if(td->type == LTT_STRING || td->type == LTT_SEQUENCE)
  //    fld->field_size = 0;
  //  else fld->field_size = -1;

  if(td->type == LTT_ENUM){
    fld->field_type->element_number = td->labels.position;
    fld->field_type->enum_strings = g_new(GQuark,td->labels.position);
    for(i=0;i<td->labels.position;i++){
      fld->field_type->enum_strings[i] 
                     = g_quark_from_string(((char*)(td->labels.array[i])));
    }
  }else if(td->type == LTT_ARRAY || td->type == LTT_SEQUENCE){
    if(td->type == LTT_ARRAY)
      fld->field_type->element_number = (unsigned)td->size;
    fld->field_type->element_type = g_new(LttType*,1);
    tmpTd = td->nested_type;
    fld->field_type->element_type[0] = lookup_named_type(fac, tmpTd);
    fld->child = g_new(LttField*, 1);
    fld->child[0] = g_new(LttField, 1);
    
//    fld->child[0]->field_pos = 0;
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
    construct_types_and_fields(fac, tmpTd, fld->child[0]);
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
	          = g_quark_from_string(((type_fields*)(td->fields.array[i]))->name);
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
      construct_types_and_fields(fac, tmpTd, fld->child[i]);
    }    
  }
}
#endif //0

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
  GQuark name = g_quark_from_string(td->type_name);
  
  LttType *type = g_datalist_id_get_data(&fac->named_types, name);

  if(type == NULL){
    type = g_new(LttType,1);
    type->type_name = name;
    g_datalist_id_set_data_full(&fac->named_types, name,
        type, (GDestroyNotify)freeLttNamedType);
    type->type_class = td->type;
    if(td->fmt) type->fmt = g_strdup(td->fmt);
    else type->fmt = NULL;
    type->size = td->size;
    type->enum_strings = NULL;
    type->element_type = NULL;
    type->element_number = 0;
  }

  return type;
}


/*****************************************************************************
 *Function name
 *    ltt_facility_close      : close a facility, decrease its usage count,
 *                              if usage count = 0, release the memory
 *Input params
 *    f                       : facility that will be closed
 ****************************************************************************/

void ltt_facility_close(LttFacility *f)
{
  //release the memory it occupied
  freeFacility(f);
}

/*****************************************************************************
 * Functions to release the memory occupied by the facility
 ****************************************************************************/

void freeFacility(LttFacility * fac)
{
  guint i;
  LttEventType *et;

  for(i=0; i<fac->events->len; i++) {
    et = &g_array_index (fac->events, LttEventType, i);
    freeEventtype(et);
  }
  g_array_free(fac->events, TRUE);

  g_datalist_clear(&fac->named_types);

}

void freeEventtype(LttEventType * evType)
{
  LttType * root_type;
  if(evType->description)
    g_free(evType->description); 
  if(evType->root_field){    
    root_type = evType->root_field->field_type;
    freeLttField(evType->root_field);
    freeLttType(&root_type);
  }
}

void freeLttNamedType(LttType * type)
{
  freeLttType(&type);
}

void freeLttType(LttType ** type)
{
  unsigned int i;
  if(*type == NULL) return;
  //if((*type)->type_name){
  //  return; //this is a named type
  //}
  if((*type)->fmt)
    g_free((*type)->fmt);
  if((*type)->enum_strings){
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
 *    f                       : the facility
 *Return value
 *    GQuark                  : the facility's name
 ****************************************************************************/

GQuark ltt_facility_name(LttFacility *f)
{
  return f->name;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_checksum   : obtain the facility's checksum
 *Input params
 *    f                       : the facility
 *Return value
 *                            : the checksum of the facility 
 ****************************************************************************/

guint32 ltt_facility_checksum(LttFacility *f)
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

guint ltt_facility_id(LttFacility *f)
{
  return f->id;
}

/*****************************************************************************
 *Function name
 *    ltt_facility_eventtype_number: obtain the number of the event types 
 *Input params
 *    f                            : the facility that will be closed
 *Return value
 *                                 : the number of the event types 
 ****************************************************************************/

guint8 ltt_facility_eventtype_number(LttFacility *f)
{
  return (f->events->len);
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

LttEventType *ltt_facility_eventtype_get(LttFacility *f, guint8 i)
{
  g_assert(i < f->events->len);
  return &g_array_index(f->events, LttEventType, i);
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

LttEventType *ltt_facility_eventtype_get_by_name(LttFacility *f, GQuark name)
{
  LttEventType *et = g_datalist_get_data(&f->events_by_name, name);
}

