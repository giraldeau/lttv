
/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
 *               2005 Mathieu Desnoyers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
LttType * lookup_named_type(LttFacility *fac, type_descriptor_t * td);

/* construct directed acyclic graph for types, and tree for fields */
void construct_fields(LttFacility *fac,
                      LttField *field,
                      field_t *fld);

/* generate the facility according to the events belongin to it */
void generateFacility(LttFacility * f, facility_t  * fac, 
                      guint32 checksum);

/* functions to release the memory occupied by a facility */
void freeFacility(LttFacility * facility);
void freeEventtype(LttEventType * evType);
void freeLttType(LttType * type);
void freeLttField(LttField * fld);
void freeLttNamedType(LttType * type);


/*****************************************************************************
 *Function name
 *    parse_fmt               : parses the format string
 *Input params
 *    fmt                     : format string specified in the xml file
 *    header                  : points to the extracted header string
 *    separator               : points to the extracted separator string
 *    footer                  : points to the extracted footer string
 *
 *    Parses the format string in order to extract the header,
 *    the separator and the footer.
 *    The default fmt string is: { %S, %S }
 *    In this case:
 *    The header is: {
 *    The separator is: ,
 *    The footer
 *
 ****************************************************************************/

parse_fmt(char *fmt, char **header, char **separator, char **footer){
  int i;
  unsigned int num = 0;

  int header_index = 0;//header index
  int separator_index = 0;//separator index
  int footer_index = 0;//footer index
  int fmt_length = strlen(fmt);

  for(i=0; i<fmt_length; i++)
    {
      if(fmt[i]=='%')
	{
	  num++;
	  if(num>=3)
	    g_error("More than 2 '%%' chars were encountered: %s\n",fmt);

	  //Detecting identifier
	  if(fmt[++i]!='S')
	    g_error("Unexpected format: %s\n",fmt);

	  if(i+1<strlen(fmt))
	    i = i+1;
	  else
	    g_error("Unexpected format: %s\n",fmt);

	  //Need to have at least on char after the %S
	  if(fmt[i]=='%')
	    g_error("Missing separator: %s\n",fmt);

	  if(separator_index==0)
	    separator_index = i;//index of separator inside the original string.
	  else if(footer_index==0)//the 'else' gives the priority to set the separator index first.
	    footer_index = i;//no break since an error should be generated if more than 2 '%' were encountered
	}
    }


  //Create header String
  num =  separator_index-header_index-2;//(-2 due to '%S')
  if(num<0)
    g_error("Unexpected format");
  //*header = malloc(num+1);//(+1 for EOS).
  *header = g_new(gchar,num+1);//(+1 for EOS).
  strncpy(*header,fmt,num);
  (*header)[num] = '\0';//need EOS, not handled by strncopy

  //Create seperator String
  num =footer_index - separator_index - 2;
  if(num<0)
    g_error("Unexpected format");
  //*separator = malloc(num+1);//+1 for EOS
  *separator = g_new(gchar, num+1);//+1 for EOS
  strncpy(*separator,fmt+separator_index*sizeof(char),num);
  (*separator)[num] = '\0';//need EOS, not handled by strncopy

  //Create footer String
  num = strlen(fmt)-footer_index;
  //*footer = malloc(num+1);
  *footer = g_new(gchar, num+1);
  strncpy(*footer,fmt+footer_index*sizeof(char),num);
  (*footer)[num] = '\0';
}

/*****************************************************************************
 *Function name
 *    verify_fmt_syntax       : verifies the syntax of the format string
 *Input params
 *    fmt                     : format string specified in the xml file
 *    fmt_type                : points to the detected type in the fmt string
 *
 *    Verifies the syntax of the format string sepcified in the xml file
 *    It allows fmt strings having the following syntax:
 *    "..header text... %12.20d ...footer text..."
 *    It sets the fmt_type accordingly. In the previous example
 *    fmt_type would point to the character 'd' inside fmt.
 *
 *returns 1 on success, and -1 or -2 on error.
 *
 ****************************************************************************/

int verify_fmt_syntax(char *fmt, char **fmt_type){
  int i;
  int dot = 0;//number of dots encountered.
  unsigned short counter = 0;
  *fmt_type=NULL;


  for(i=0; i<strlen(fmt); i++)
    {
      if(fmt[i]=='%')
	{
	  if(++counter==2)
	    return -1;//should generate an error.

	  i = i+1;//go to next character

	  if(fmt[i]=='-' && i<(strlen(fmt)-1))
	    i++;//allow this character after the '%'

	  if(fmt[i]=='#' && i<(strlen(fmt)-1))
	    i++;//allow this character
	}

      if(counter ==1 && !isdigit(fmt[i]) && *fmt_type==NULL)
	     switch (fmt[i]){
	     case 'd':
	     case 'i':
	     case 'u':
	     case 'o':
	     case 'x':
	     case 'X':
	     case 'f':
	     case 'e':
	     case 'E':
	     case 's':
	     case 'p':
	       *fmt_type=&fmt[i];
	       break;
	       //do not return yet. may encounter another '%'
	     case '.':
	       if(++dot==2)
		 return -2;//generate error
	       break;
	     default:
	       return -1;
	     }

    }

  return 1;

}

/*****************************************************************************
 *Function name
 *    append_ll               : appends "ll" to the format string
 *Input params
 *    fmt                     : format string specified in the xml file
 *    fmt_type                : address of the format char inside fmt
 *
 *  inserts "ll" just before the fmt_type
 *
 ****************************************************************************/

void append_ll (char **fmt, char **fmt_type){
  char *new_fmt;
  int i;
  int num;


  //the +2 corresponds the the "ll"; the +1 is the \0
  new_fmt = g_new(gchar, strlen(*fmt)+2+1);

 num = *fmt_type - *fmt;

 for(i=0; i<num;i++)
   new_fmt[i] =(*fmt)[i];

  new_fmt[i++] = 'l';
  new_fmt[i++] = 'l';


  while(i-2<strlen(*fmt))
    {
      new_fmt[i]=(*fmt)[i-2];
      i++;
    }

  *fmt = new_fmt;
  (*fmt)[i] = '\0';

}


/*****************************************************************************
 *Function name
 *    ltt_facility_open       : open facilities
 *Input params
 *    t                       : the trace containing the facilities
 *    pathname                : the path name of the facility   
 *
 *  Open the facility corresponding to the right checksum.
 * 
 *returns 0 on success, 1 on error.
 ****************************************************************************/

int ltt_facility_open(LttFacility *f, LttTrace * t, gchar * pathname)
{
  int ret = 0;
  gchar *token;
  parse_file_t in;
  facility_t * fac;
  unsigned int checksum;
  gchar buffer[BUFFER_SIZE];
  gboolean generated = FALSE;

  in.buffer = &(buffer[0]);
  in.lineno = 1;
  in.error = error_callback;
  in.name = pathname;
  in.unget = 0;

  in.fp = fopen(in.name, "r");
  if(in.fp == NULL) {
    g_warning("cannot open facility description file %s",
        in.name);
    ret = 1;
    goto open_error;
  }

  while(1){
    token = getToken(&in);
    if(in.type == ENDFILE) break;
   
    if(g_ascii_strcasecmp(token, "<")) in.error(&in,"not a facility file");
    token = getName(&in);
    if(g_ascii_strcasecmp(token, "?")) in.error(&in,"not a facility file");
    token = getName(&in);
    if(g_ascii_strcasecmp(token, "xml")) in.error(&in,"not a facility file");
    token = getName(&in);
    if(g_ascii_strcasecmp(token, "version")) in.error(&in,"not a facility file");
    token = getName(&in);
    if(g_ascii_strcasecmp(token, "=")) in.error(&in,"not a facility file");
    token = getQuotedString(&in);
    if(g_ascii_strcasecmp(token, "1.0")) in.error(&in,"not a facility file");
    token = getName(&in);
    if(g_ascii_strcasecmp(token, "?")) in.error(&in,"not a facility file");
    token = getToken(&in);
    if(g_ascii_strcasecmp(token, ">")) in.error(&in,"not a facility file");

    token = getToken(&in);
    
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
      checkNamedTypesImplemented(&fac->named_types);
    
      generateChecksum(fac->name, &checksum, &fac->events);
      // FIXME if(checksum == f->checksum) {
        generateFacility(f, fac, checksum);
        generated = TRUE;
      //}
      if (checksum != f->checksum)
      	g_warning("Facility checksum mismatch for facility %s : kernel 0x%X vs "
		"XML 0x%X\n", fac->name, f->checksum, checksum);

      g_free(fac->name);
      free(fac->capname);
      g_free(fac->description);
      freeEvents(&fac->events);
      sequence_dispose(&fac->events);
      freeNamedType(&fac->named_types);
      table_dispose(&fac->named_types);
      freeTypes(&fac->unnamed_types);
      sequence_dispose(&fac->unnamed_types);      
      g_free(fac);
      if(generated) break; /* use the first good match */
    }
    else {
      g_warning("facility token was expected in file %s", in.name);
      ret = 1;
      goto parse_error;
    }
  }
  
 parse_error:
  fclose(in.fp);
open_error:

  if(!generated) {
    g_warning("Cannot find facility %s, checksum 0x%X",
        g_quark_to_string(f->name), f->checksum);
    ret = 1;
  }

  return ret;
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
  sequence_t * events = &fac->events;
  unsigned int i, j;
  LttType * type;
  table_t *named_types = &fac->named_types;
  
  g_assert(f->name == g_quark_from_string(facilityName));
  //g_assert(f->checksum == checksum);

  //f->event_number = events->position;
  
  //initialize inner structures
  f->events = g_array_sized_new (FALSE, TRUE, sizeof(LttEventType),
      events->position);
  //f->events = g_new(LttEventType*,f->event_number);
  f->events = g_array_set_size(f->events, events->position);

  g_datalist_init(&f->events_by_name);
 // g_datalist_init(&f->named_types);
#if 0
  /* The first day, he created the named types */

  for(i=0; i<named_types->keys.position; i++) {
    GQuark name = g_quark_from_string((char*)named_types->keys.array[i]);
    type_descriptor_t *td = (type_descriptor_t*)named_types->values.array[i];

    /* Create the type */
    type = g_new(LttType,1);
    type->type_name = name;
    type->type_class = td->type;
    if(td->fmt) type->fmt = g_strdup(td->fmt);
    else type->fmt = NULL;
    type->size = td->size;
    type->enum_strings = NULL;
    type->element_type = NULL;
    type->element_number = 0;

    construct_types_and_fields(type, td, NULL, NULL, ...);
    
    g_datalist_id_set_data_full(&fac->named_types, name,
                type, (GDestroyNotify)freeLttNamedType);

  }
#endif //0
  /* The second day, he created the event fields and types */
  //for each event, construct field and type acyclic graph
  for(i=0;i<events->position;i++){
    event_t *parser_event = (event_t*)events->array[i];
    LttEventType *event_type = &g_array_index(f->events, LttEventType, i);

    event_type->name = 
      g_quark_from_string(parser_event->name);
    
    g_datalist_id_set_data(&f->events_by_name, event_type->name,
        event_type);
    
    event_type->description =
      g_strdup(parser_event->description);
    
    event_type->index = i;
    event_type->facility = f;

    event_type->has_compact_data = parser_event->compact_data;

    event_type->fields = g_array_sized_new(FALSE, TRUE,
        sizeof(LttField), parser_event->fields.position);
    event_type->fields = 
      g_array_set_size(event_type->fields, parser_event->fields.position);
    g_datalist_init(&event_type->fields_by_name);
  
    for(j=0; j<parser_event->fields.position; j++) {
      LttField *field = &g_array_index(event_type->fields, LttField, j);
      field_t *parser_field = (field_t*)parser_event->fields.array[j];

      construct_fields(f, field, parser_field);
      g_datalist_id_set_data(&event_type->fields_by_name, 
         field->name, 
         field);
    }
  }

  /* What about 2 days weeks ? */
}


/*****************************************************************************
 *Function name
 *    construct_types_and_fields : construct field tree and type graph,
 *                             internal recursion function
 *Input params 
 *    fac                    : facility struct
 *    field                  : destination lttv field
 *    fld                    : source parser field
 ****************************************************************************/

//DONE
//make the change for arrays and sequences
//no more root field. -> change this for an array of fields.
// Compute the field size here.
// Flag fields as "VARIABLE OFFSET" or "FIXED OFFSET" : as soon as
// a field with a variable size is found, all the following fields must
// be flagged with "VARIABLE OFFSET", this will be done by the offset
// precomputation.


void construct_fields(LttFacility *fac,
                      LttField *field,
                      field_t *fld)
{
  char *fmt_type;//gaby
  guint len;
  type_descriptor_t *td;
  LttType *type;

  if(fld->name)
    field->name = g_quark_from_string(fld->name);
  else
    fld->name = 0;

  if(fld->description) {
    len = strlen(fld->description);
    field->description = g_new(gchar, len+1);
    strcpy(field->description, fld->description);
  }
  field->dynamic_offsets = NULL;
  type = &field->field_type;
  td = fld->type;

  type->enum_map = NULL;
  type->fields = NULL;
  type->fields_by_name = NULL;
  type->network = td->network;

  switch(td->type) {
    case INT_FIXED:
      type->type_class = LTT_INT_FIXED;
      type->size = td->size;
      break;
    case UINT_FIXED:
      type->type_class = LTT_UINT_FIXED;
      type->size = td->size;
      break;
    case POINTER:
      type->type_class = LTT_POINTER;
      type->size = fac->pointer_size;
      break;
    case CHAR:
      type->type_class = LTT_CHAR;
      type->size = td->size;
      break;
    case UCHAR:
      type->type_class = LTT_UCHAR;
      type->size = td->size;
      g_assert(type->size != 0);
      break;
    case SHORT:
      type->type_class = LTT_SHORT;
      type->size = td->size;
      break;
    case USHORT:
      type->type_class = LTT_USHORT;
      type->size = td->size;
      break;
    case INT:
      type->type_class = LTT_INT;
      type->size = fac->int_size;
      break;
    case UINT:
      type->type_class = LTT_UINT;
      type->size = fac->int_size;
      g_assert(type->size != 0);
      break;
    case LONG:
      type->type_class = LTT_LONG;
      type->size = fac->long_size;
      break;
    case ULONG:
      type->type_class = LTT_ULONG;
      type->size = fac->long_size;
      break;
    case SIZE_T:
      type->type_class = LTT_SIZE_T;
      type->size = fac->size_t_size;
      break;
    case SSIZE_T:
      type->type_class = LTT_SSIZE_T;
      type->size = fac->size_t_size;
      break;
    case OFF_T:
      type->type_class = LTT_OFF_T;
      type->size = fac->size_t_size;
      break;
    case FLOAT:
      type->type_class = LTT_FLOAT;
      type->size = td->size;
      break;
    case STRING:
      type->type_class = LTT_STRING;
      type->size = 0;
      break;
    case ENUM:
      type->type_class = LTT_ENUM;
      type->size = fac->int_size;
      {
        guint i;
        type->enum_map = g_hash_table_new(g_direct_hash, g_direct_equal);
	type->lowest_value = G_MAXINT32;
	type->highest_value = G_MININT32;
        for(i=0; i<td->labels.position; i++) {
          GQuark value = g_quark_from_string((char*)td->labels.array[i]);
          gint key = *(int*)td->labels_values.array[i];
          g_hash_table_insert(type->enum_map, (gpointer)key, (gpointer)value);
	  type->highest_value = max(key, type->highest_value);
	  type->lowest_value = min(key, type->lowest_value);
        }
      }
      g_assert(type->size != 0);
      break;
    case ARRAY:
      type->type_class = LTT_ARRAY;
      type->size = td->size;
      type->fields = g_array_sized_new(FALSE, TRUE, sizeof(LttField),
          td->fields.position);
      type->fields = g_array_set_size(type->fields, td->fields.position);
      {
        guint i;

        for(i=0; i<td->fields.position; i++) {
          field_t *schild = (field_t*)td->fields.array[i];
          LttField *dchild = &g_array_index(type->fields, LttField, i);
          
          construct_fields(fac, dchild, schild);
        }
      }
      break;
    case SEQUENCE:
      type->type_class = LTT_SEQUENCE;
      type->size = 0;
      type->fields = g_array_sized_new(FALSE, TRUE, sizeof(LttField),
          td->fields.position);
      type->fields = g_array_set_size(type->fields, td->fields.position);
      {
        guint i;

        for(i=0; i<td->fields.position; i++) {
          field_t *schild = (field_t*)td->fields.array[i];
          LttField *dchild = &g_array_index(type->fields, LttField, i);
          
          construct_fields(fac, dchild, schild);
        }
      }
      break;
    case STRUCT:
      type->type_class = LTT_STRUCT;
      type->size = 0; // Size not calculated by the parser.
      type->fields = g_array_sized_new(FALSE, TRUE, sizeof(LttField),
          td->fields.position);
      type->fields = g_array_set_size(type->fields, td->fields.position);
      g_datalist_init(&type->fields_by_name);
      {
        guint i;

        for(i=0; i<td->fields.position; i++) {
          field_t *schild = (field_t*)td->fields.array[i];
          LttField *dchild = &g_array_index(type->fields, LttField, i);
          
          construct_fields(fac, dchild, schild);
          g_datalist_id_set_data(&type->fields_by_name, 
             dchild->name, 
             dchild);
        }
      }
      break;
    case UNION:
      type->type_class = LTT_UNION;
      type->size = 0; // Size not calculated by the parser.
      type->fields = g_array_sized_new(FALSE, TRUE, sizeof(LttField),
          td->fields.position);
      type->fields = g_array_set_size(type->fields, td->fields.position);
      g_datalist_init(&type->fields_by_name);
      {
        guint i;

        for(i=0; i<td->fields.position; i++) {
          field_t *schild = (field_t*)td->fields.array[i];
          LttField *dchild = &g_array_index(type->fields, LttField, i);
          
          construct_fields(fac, dchild, schild);
          g_datalist_id_set_data(&type->fields_by_name, 
             dchild->name, 
             dchild);
        }
      }
      break;
    case NONE:
    default:
      g_error("construct_fields : unknown type");
  }

  field->field_size = type->size;

  /* Put the fields as "variable" offset to root first. Then,
   * the offset precomputation will only have to set the FIELD_FIXED until
   * it reaches the first variable length field, then stop.
   */
  field->fixed_root = FIELD_VARIABLE;

  if(td->fmt) {
    len = strlen(td->fmt);
    type->fmt = g_new(gchar, len+1);
    strcpy(type->fmt, td->fmt);
  }
    //here I should verify syntax based on type.
    //if type is array or sequence or enum, parse_fmt.
    //if type is basic, verify syntax, and allow or not the type format.
    //the code can be integrated in the above code (after testing)



   switch (type->type_class){
    case LTT_ARRAY:
    case LTT_UNION:
    case LTT_STRUCT:
    case LTT_SEQUENCE:
      if(type->fmt==NULL)
	{
	  //Assign a default format for these complex types
	  //type->fmt = malloc(11*sizeof(char));
	  type->fmt = g_new(gchar, 11);
	  type->fmt = g_strdup("{ %S, %S }");//set a default value for fmt. can directly set header and footer, but kept this way on purpose.
	}
      //Parse the fmt string in order to extract header, separator and footer
      parse_fmt(type->fmt,&(type->header),&(type->separator),&(type->footer));

      break;
    case LTT_SHORT:
    case LTT_INT:
    case LTT_LONG:
    case LTT_SSIZE_T:
    case LTT_INT_FIXED:

     if(type->fmt == NULL)
	{
	  //Assign a default format string
	  //type->fmt = malloc(5*sizeof(char));
	  type->fmt = g_new(gchar, 5);
	  type->fmt=g_strdup("%lld");
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>0)
	  switch(fmt_type[0]){
	  case 'd':
	  case 'i':
	  case 'x':
	  case 'X':
	    append_ll(&(type->fmt),&fmt_type);//append 'll' to fmt
	    break;
	  default:
	    g_error("Format type '%c' not supported\n",fmt_type[0]);
	    break;
	  }
     break;

    case LTT_USHORT:
    case LTT_UINT:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_OFF_T:
    case LTT_UINT_FIXED:
      if(type->fmt == NULL)
	{
	  //Assign a default format string
	  //type->fmt= malloc(5*sizeof(char));
	  type->fmt = g_new(gchar, 5);
	  type->fmt=g_strdup("%lld");
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>0)
	  switch(fmt_type[0]){
	  case 'd':
	  case 'u':
	  case 'o':
	  case 'x':
	  case 'X':
	    append_ll(&(type->fmt),&fmt_type);
	    break;
	  default:
	    g_error("Format type '%c' not supported\n",fmt_type[0]);
	  }
      break;

    case LTT_CHAR:
    case LTT_UCHAR:
      if(type->fmt == NULL)
	{
	  //Assign a default format string
	  //type->fmt = malloc(3*sizeof(char));
	  type->fmt = g_new(gchar, 3);
	  type->fmt = g_strdup("%c");
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>1)
	  switch(fmt_type[0]){
	  case 'c':
	  case 'd':
	  case 'u':
	  case 'x':
	  case 'X':
	  case 'o':
	    break;
	  default:
	    g_error("Format type '%c' not supported\n",fmt_type[0]);
	  }
      break;

    case LTT_FLOAT:
      if(type->fmt == NULL)
	{
	  //Assign a default format string
	  //type->fmt = malloc(3*sizeof(char));
	  type->fmt = g_new(gchar, 3);
	  type->fmt = g_strdup("%g");
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>0)
	  switch(fmt_type[0]){
	  case 'f':
	  case 'g':
	  case 'G':
	  case 'e':
	  case 'E':
	    break;
	  default:
	    g_error("Format type '%c' not supported\n",fmt_type[0]);
	  }
      break;

    case LTT_POINTER:
      if(type->fmt == NULL)
	{
	  //Assign a default format string
	  //type->fmt = malloc(7*sizeof(char));
	  type->fmt = g_new(gchar, 7);
	  type->fmt = g_strdup("0x%llx");
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>0)
	  switch(fmt_type[0]){
	  case 'p':
	    //type->fmt = malloc(7*sizeof(char));
	    type->fmt = g_new(gchar, 7);
	    type->fmt = g_strdup("0x%llx");
	    break;
	  case 'x':
	  case 'X':
	  case 'd':
	    append_ll(&(type->fmt),&fmt_type);
	    break;
	  default:
	    g_error("Format type '%c' not supported\n",fmt_type[0]);
	  }
      break;

    case LTT_STRING:
      if(type->fmt == NULL)
	{
	  //type->fmt = malloc(7*sizeof(char));
	  type->fmt = g_new(gchar, 5);
	  type->fmt = g_strdup("\"%s\"");//default value for fmt.
	  break;
	}
      else
	if(verify_fmt_syntax((type->fmt),&fmt_type)>0)
	  switch(fmt_type[0]){
	  case 's':
	    break;
	  default:
	    g_error("Format type '%c' not supported\n", fmt_type[0]);
	  }
      break;

   default:
     //missing enum
     break;
   }


}

#if 0
void construct_types_and_fields(LttFacility * fac, type_descriptor_t * td, 
                            LttField * fld)
{
  int i;
  type_descriptor_t * tmpTd;

  switch(td->type) {
    case INT:
    case UINT:
    case FLOAT:
      fld->field_type->size = td->size;
      break;
    case POINTER:
    case LONG:
    case ULONG:
    case SIZE_T:
    case SSIZE_T:
    case OFF_T:
      fld->field_type->size = 0;
      break;
    case STRING:
      fld->field_type->size = 0;
      break;
    case ENUM:
      fld->field_type->element_number = td->labels.position;
      fld->field_type->enum_strings = g_new(GQuark,td->labels.position);
      for(i=0;i<td->labels.position;i++){
        fld->field_type->enum_strings[i] 
                       = g_quark_from_string(((char*)(td->labels.array[i])));
      }
      fld->field_type->size = td->size;
      break;

    case ARRAY:
      fld->field_type->element_number = (unsigned)td->size;
    case SEQUENCE:
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

  case STRUCT:
  case UNION:
    fld->field_type->element_number = td->fields.position;

    g_assert(fld->field_type->element_type == NULL);
    fld->field_type->element_type = g_new(LttType*, td->fields.position);

    fld->child = g_new(LttField*, td->fields.position);      
    for(i=0;i<td->fields.position;i++){
      tmpTd = ((field_t*)(td->fields.array[i]))->type;

      fld->field_type->element_type[i] = lookup_named_type(fac, tmpTd);
      fld->child[i] = g_new(LttField,1); 

 //     fld->child[i]->field_pos = i;
      fld->child[i]->field_type = fld->field_type->element_type[i]; 

      fld->child[i]->field_type->element_name 
            = g_quark_from_string(((field_t*)(td->fields.array[i]))->name);

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

#endif //0

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

#if 0
/*****************************************************************************
 *Function name
 *    lookup_named_type: search named type in the table
 *                       internal function
 *Input params 
 *    fac              : facility struct
 *    name             : type name
 *Return value    
 *                     : either find the named type, or create a new LttType
 ****************************************************************************/

LttType * lookup_named_type(LttFacility *fac, GQuark type_name)
{
  LttType *type = NULL;
  
  /* Named type */
  type = g_datalist_id_get_data(&fac->named_types, name);

  g_assert(type != NULL);
#if 0
  if(type == NULL){
    /* Create the type */
    type = g_new(LttType,1);
    type->type_name = name;
    type->type_class = td->type;
    if(td->fmt) type->fmt = g_strdup(td->fmt);
    else type->fmt = NULL;
    type->size = td->size;
    type->enum_strings = NULL;
    type->element_type = NULL;
    type->element_number = 0;
    
    if(td->type_name != NULL)
      g_datalist_id_set_data_full(&fac->named_types, name,
                  type, (GDestroyNotify)freeLttNamedType);
  }
#endif //0
  return type;
}
#endif //0

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

  g_datalist_clear(&fac->events_by_name);

 // g_datalist_clear(&fac->named_types);
}

void freeEventtype(LttEventType * evType)
{
  unsigned int i;
  LttType * root_type;
  if(evType->description)
    g_free(evType->description);
  
  for(i=0; i<evType->fields->len;i++) {
    LttField *field = &g_array_index(evType->fields, LttField, i);
    freeLttField(field);
  }
  g_array_free(evType->fields, TRUE);
  g_datalist_clear(&evType->fields_by_name);
}

void freeLttType(LttType * type)
{
  unsigned int i;

  if(type->fmt)
    g_free(type->fmt);

  if(type->enum_map)
    g_hash_table_destroy(type->enum_map);

  if(type->fields) {
    for(i=0; i<type->fields->len; i++) {
      freeLttField(&g_array_index(type->fields, LttField, i));
    }
    g_array_free(type->fields, TRUE);
  }
  if(type->fields_by_name)
    g_datalist_clear(&type->fields_by_name);

  if(type->header)
    g_free(type->header);//no need for condition? if(type->header)
  if(type->separator)
    g_free(type->separator);
  if(type->footer)
    g_free(type->footer);
}

void freeLttNamedType(LttType * type)
{
  freeLttType(type);
}

void freeLttField(LttField * field)
{ 
  if(field->description)
    g_free(field->description);
  if(field->dynamic_offsets)
    g_array_free(field->dynamic_offsets, TRUE);
  freeLttType(&field->field_type);
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
  if(!f->exists) return NULL;

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
  LttEventType *et = g_datalist_id_get_data(&f->events_by_name, name);
  return et;
}

