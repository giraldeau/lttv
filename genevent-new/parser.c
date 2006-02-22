/*

parser.c: Generate helper declarations and functions to trace events
  from an event description file.

	    Copyright (C) 2005, Mathieu Desnoyers
      Copyright (C) 2002, Xianxiu Yang
      Copyright (C) 2002, Michel Dagenais 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/* This program reads the ".xml" event definitions input files 
   and constructs structure for each event.
 
   The program uses a very simple tokenizer, called from a hand written
   recursive descent parser to fill a data structure describing the events.
   The result is a sequence of events definitions which refer to type
   definitions.

   A table of named types is maintained to allow refering to types by name
   when the same type is used at several places. Finally a sequence of
   all types is maintained to facilitate the freeing of all type 
   information when the processing of an ".xml" file is finished. */

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <linux/errno.h>  
#include <assert.h>
#include <ctype.h>

#include "parser.h"


char *intOutputTypes[] = {
  "int8_t", "int16_t", "int32_t", "int64_t" };

char *uintOutputTypes[] = {
  "uint8_t", "uint16_t", "uint32_t", "uint64_t" };

char *floatOutputTypes[] = {
  "undef", "undef", "float", "double" };




/* helper function  */
void strupper(char *string)
{
  char *ptr = string;
  
  while(*ptr != '\0') {
    *ptr = toupper(*ptr);
		ptr++;
  }
}


int getSizeindex(unsigned int value)
{ 
  switch(value) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      printf("Error : unknown value size %d\n", value);
      exit(-1);
  }
}

/*****************************************************************************
 *Function name
 *    getSize    : translate from string to integer
 *Input params 
 *    in         : input file handle
 *Return values  
 *    size                           
 *****************************************************************************/

unsigned long long int getSize(parse_file_t *in)
{
  char *token;

  token = getToken(in);
  if(in->type == NUMBER) {
		return strtoull(token, NULL, 0);
  }
  in->error(in,"incorrect size specification");
  return -1;
}

/*****************************************************************************
 *Function name
 *    error_callback  : print out error info
 *Input params
 *    in              : input file handle
 *    msg             : message to be printed                  
 ****************************************************************************/

void error_callback(parse_file_t *in, char *msg)
{
  if(in)
    printf("Error in file %s, line %d: %s\n", in->name, in->lineno, msg);
  else
    printf("%s\n",msg);
  assert(0);
  exit(1);
}

/*****************************************************************************
 *Function name
 *    memAlloc  : allocate memory                    
 *Input params 
 *    size      : required memory size               
 *return value 
 *    void *    : pointer to allocate memory or NULL 
 ****************************************************************************/

void * memAlloc(int size)
{
  void * addr;
  if(size == 0) return NULL;
  addr = malloc(size);
  if(!addr){
    printf("Failed to allocate memory");    
    exit(1);
  }
  return addr;	  
}

/*****************************************************************************
 *Function name
 *    allocAndCopy : allocate memory and initialize it  
 *Input params 
 *    str          : string to be put in memory         
 *return value 
 *    char *       : pointer to allocate memory or NULL
 ****************************************************************************/

char *allocAndCopy(char *str)
{
  char * addr;
  if(str == NULL) return NULL;
  addr = (char *)memAlloc(strlen(str)+1);
  strcpy(addr,str);
  return addr;
}

/**************************************************************************
 * Function :
 *    getTypeAttributes
 * Description :
 *    Read the attribute from the input file.
 *
 * Parameters :
 *    in , input file handle.
 *    t , the type descriptor to fill.
 *
 **************************************************************************/

void getTypeAttributes(parse_file_t *in, type_descriptor_t *t,
			   sequence_t * unnamed_types, table_t * named_types) 
{
  char * token;

  t->fmt = NULL;
  t->size = 0;
  t->custom_write = 0;
	t->network = 0;
  
  while(1) {
    token = getToken(in); 
    if(strcmp("/",token) == 0 || strcmp(">",token) == 0){
      ungetToken(in);
      break;
    }
    
    if(!strcmp("format",token)) {
      getEqual(in);
      t->fmt = allocAndCopy(getQuotedString(in));
    //} else if(!strcmp("name",token)) {
     // getEqual(in);
     // car = seekNextChar(in);
     // if(car == EOF) in->error(in,"name was expected");
     // else if(car == '\"') t->type_name = allocAndCopy(getQuotedString(in));
     // else t->type_name = allocAndCopy(getName(in));
    } else if(!strcmp("size",token)) {
      getEqual(in);
      t->size = getSize(in);
    } else if(!strcmp("custom_write", token)) {
      t->custom_write = 1;
    } else if(!strcmp("network", token)) {
			t->network = 1;
		}
  }
}

/**************************************************************************
 * Function :
 *    getEventAttributes
 * Description :
 *    Read the attribute from the input file.
 *
 * Parameters :
 *    in , input file handle.
 *    ev , the event to fill.
 *
 **************************************************************************/

void getEventAttributes(parse_file_t *in, event_t *ev)
{
  char * token;
  char car;
  
  ev->name = NULL;
  ev->per_trace = 0;
  ev->per_tracefile = 0;

  while(1) {
    token = getToken(in); 
    if(strcmp("/",token) == 0 || strcmp(">",token) == 0){
      ungetToken(in);
      break;
    }

    if(!strcmp("name",token)) {
      getEqual(in);
      car = seekNextChar(in);
      if(car == EOF) in->error(in,"name was expected");
      else if(car == '\"') ev->name = allocAndCopy(getQuotedString(in));
      else ev->name = allocAndCopy(getName(in));
    } else if(!strcmp("per_trace", token)) {
      ev->per_trace = 1;
    } else if(!strcmp("per_tracefile", token)) {
      ev->per_tracefile = 1;
    }

  }
}

/**************************************************************************
 * Function :
 *    getFacilityAttributes
 * Description :
 *    Read the attribute from the input file.
 *
 * Parameters :
 *    in , input file handle.
 *    fac , the facility to fill.
 *
 **************************************************************************/

void getFacilityAttributes(parse_file_t *in, facility_t *fac)
{
  char * token;
  char car;
  
  fac->name = NULL;
  fac->arch = NULL;

  while(1) {
    token = getToken(in); 
    if(strcmp("/",token) == 0 || strcmp(">",token) == 0){
      ungetToken(in);
      break;
    }

    if(!strcmp("name",token)) {
      getEqual(in);
      car = seekNextChar(in);
      if(car == EOF) in->error(in,"name was expected");
      else if(car == '\"') fac->name = allocAndCopy(getQuotedString(in));
      else fac->name = allocAndCopy(getName(in));
    } else if(!strcmp("arch", token)) {
      getEqual(in);
      car = seekNextChar(in);
      if(car == '\"') fac->name = allocAndCopy(getQuotedString(in));
			else fac->arch = allocAndCopy(getName(in));
		}
  }
}

/**************************************************************************
 * Function :
 *    getFieldAttributes
 * Description :
 *    Read the attribute from the input file.
 *
 * Parameters :
 *    in , input file handle.
 *    f , the field to fill.
 *
 **************************************************************************/

void getFieldAttributes(parse_file_t *in, field_t *f)
{
  char * token;
  char car;

  f->name = NULL;

  while(1) {
    token = getToken(in); 
    if(strcmp("/",token) == 0 || strcmp(">",token) == 0){
      ungetToken(in);
      break;
    }

    if(!strcmp("name",token)) {
      getEqual(in);
      car = seekNextChar(in);
      if(car == EOF) in->error(in,"name was expected");
      else if(car == '\"') f->name = allocAndCopy(getQuotedString(in));
      else f->name = allocAndCopy(getName(in));
    }
	}
}

char *getNameAttribute(parse_file_t *in)
{
  char * token;
  char *name = NULL;
  char car;
  
  while(1) {
    token = getToken(in); 
    if(!strcmp("name",token)) {
      getEqual(in);
      car = seekNextChar(in);
      if(car == EOF) in->error(in,"name was expected");
      else if(car == '\"') name = allocAndCopy(getQuotedString(in));
      else name = allocAndCopy(getName(in));
    } else {
      ungetToken(in);
      break;
    }

  }
  if(name == NULL) in->error(in, "Name was expected");
  return name;
}



//for <label name=label_name value=n format="..."/>, value is an option
char * getValueStrAttribute(parse_file_t *in)
{
  char * token;

  token = getToken(in); 
  if(strcmp("/",token) == 0){
    ungetToken(in);
    return NULL;
  }
  
  if(strcmp("value",token))in->error(in,"value was expected");
  getEqual(in);
  token = getToken(in);
  if(in->type != NUMBER) in->error(in,"number was expected");
  return token;  
}

char * getDescription(parse_file_t *in)
{
  long int pos;
  char * token, car, *str;

  pos = ftell(in->fp);

  getLAnglebracket(in);
  token = getName(in);
  if(strcmp("description",token)){
    fseek(in->fp, pos, SEEK_SET);
    return NULL;
  }
  
  getRAnglebracket(in);

  pos = 0;
  while((car = getc(in->fp)) != EOF) {
    if(car == '<') break;
    if(car == '\0') continue;
    in->buffer[pos] = car;
    pos++;
  }
  if(car == EOF)in->error(in,"not a valid description");
  in->buffer[pos] = '\0';

  str = allocAndCopy(in->buffer);

  getForwardslash(in);
  token = getName(in);
  if(strcmp("description", token))in->error(in,"not a valid description");
  getRAnglebracket(in);

  return str;
}

/*****************************************************************************
 *Function name
 *    parseFacility : generate event list  
 *Input params 
 *    in            : input file handle          
 *    fac           : empty facility
 *Output params
 *    fac           : facility filled with event list
 ****************************************************************************/

void parseFacility(parse_file_t *in, facility_t * fac)
{
  char * token;
  event_t *ev;
  
  getFacilityAttributes(in, fac);
  if(fac->name == NULL) in->error(in, "Attribute not named");
	
  fac->capname = allocAndCopy(fac->name);
	strupper(fac->capname);
  getRAnglebracket(in);    
  
  fac->description = getDescription(in);
  
  while(1){
    getLAnglebracket(in);    

    token = getToken(in);
    if(in->type == ENDFILE)
      in->error(in,"the definition of the facility is not finished");

    if(strcmp("event",token) == 0){
      ev = (event_t*) memAlloc(sizeof(event_t));
      sequence_push(&(fac->events),ev);
      parseEvent(in,ev, &(fac->unnamed_types), &(fac->named_types));    
    }else if(strcmp("type",token) == 0){
      parseTypeDefinition(in, &(fac->unnamed_types), &(fac->named_types));
    }else if(in->type == FORWARDSLASH){
      break;
    }else in->error(in,"event or type token expected\n");
  }

  token = getName(in);
  if(strcmp("facility",token)) in->error(in,"not the end of the facility");
  getRAnglebracket(in); //</facility>
}

/*****************************************************************************
 *Function name
 *    parseEvent    : generate event from event definition 
 *Input params 
 *    in            : input file handle          
 *    ev            : new event                              
 *    unnamed_types : array of unamed types
 *    named_types   : array of named types
 *Output params    
 *    ev            : new event (parameters are passed to it)   
 ****************************************************************************/

void parseEvent(parse_file_t *in, event_t * ev, sequence_t * unnamed_types, 
		table_t * named_types) 
{
  char *token;
	field_t *f;

	sequence_init(&(ev->fields));
  //<event name=eventtype_name>
  getEventAttributes(in, ev);
  if(ev->name == NULL) in->error(in, "Event not named");
  getRAnglebracket(in);  

  //<description>...</description>
  ev->description = getDescription(in); 
  
	int got_end = 0;
	/* Events can have multiple fields. each field form at least a function
	 * parameter of the logging function. */
	while(!got_end) {
		getLAnglebracket(in);
		token = getToken(in);
		
		switch(in->type) {
		case FORWARDSLASH:	/* </event> */
			token = getName(in);
			if(strcmp("event",token))in->error(in,"not an event definition");
			getRAnglebracket(in);  //</event>
			got_end = 1;
			break;
		case NAME: /* a field */
			if(strcmp("field",token))in->error(in,"expecting a field");
			f = (field_t *)memAlloc(sizeof(field_t));
			sequence_push(&(ev->fields),f);
			parseFields(in, f, unnamed_types, named_types, 1);
			break;
		default:
			in->error(in, "expecting </event> or <field >");
			break;
		}
	}
#if 0
		if(in->type == FORWARDSLASH){ //</event> NOTHING
			ev->type = NULL;
		}else if(in->type == NAME){
			if(strcmp("struct",token)==0 || strcmp("typeref",token)==0){
				ungetToken(in);
				ev->type = parseType(in,NULL, unnamed_types, named_types);
				if(ev->type->type != STRUCT && ev->type->type != NONE) 
		in->error(in,"type must be a struct");     
			}else in->error(in, "not a valid type");

			getLAnglebracket(in);
			getForwardslash(in);    
		}else in->error(in,"not a struct type");
		getLAnglebracket(in);
		getForwardslash(in);    
		token = getName(in);
		if(strcmp("event",token))in->error(in,"not an event definition");
		getRAnglebracket(in);  //</event>
#endif //0
}

/*****************************************************************************
 *Function name
 *    parseField    : get field infomation from buffer 
 *Input params 
 *    in            : input file handle
 *    f             : field
 *    unnamed_types : array of unamed types
 *    named_types   : array of named types
 *    tag						: is field surrounded by a <field> </field> tag ?
 ****************************************************************************/

void parseFields(parse_file_t *in, field_t *f,
    sequence_t * unnamed_types,
		table_t * named_types,
		int tag) 
{
  char * token;
	if(tag) {
		//<field name=field_name> <description> <type> </field>
		getFieldAttributes(in, f);
		if(f->name == NULL) in->error(in, "Field not named");
		getRAnglebracket(in);

		f->description = getDescription(in);
	} else {
		f->description = NULL;
	}

  //<int size=...>
  getLAnglebracket(in);
  f->type = parseType(in,NULL, unnamed_types, named_types);

	if(tag) {
		getLAnglebracket(in);
		getForwardslash(in);
		token = getName(in);
		if(strcmp("field",token))in->error(in,"not a valid field definition");
		getRAnglebracket(in); //</field>
	}
}


/*****************************************************************************
 *Function name
 *    parseType      : get type information, type can be : 
 *                     Primitive:
 *                        int(size,fmt); uint(size,fmt); float(size,fmt); 
 *                        string(fmt); enum(size,fmt,(label1,label2...))
 *                     Compound:
 *                        array(arraySize, type); sequence(lengthSize,type)
 *		          struct(field(name,type,description)...)
 *                     type name:
 *                        type(name,type)
 *Input params 
 *    in               : input file handle
 *    inType           : a type descriptor          
 *    unnamed_types    : array of unamed types
 *    named_types      : array of named types
 *Return values  
 *    type_descriptor* : a type descriptor             
 ****************************************************************************/

type_descriptor_t *parseType(parse_file_t *in, type_descriptor_t *inType, 
			   sequence_t * unnamed_types, table_t * named_types) 
{
  char *token;
  type_descriptor_t *t;
	field_t *f;

  if(inType == NULL) {
    t = (type_descriptor_t *) memAlloc(sizeof(type_descriptor_t));
    t->type_name = NULL;
    t->type = NONE;
    t->fmt = NULL;
    sequence_push(unnamed_types,t);
  }
  else t = inType;

  token = getName(in);

  if(strcmp(token,"struct") == 0) {
    t->type = STRUCT;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getRAnglebracket(in); //<struct>
    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(strcmp("field",token) == 0){
			f = (field_t *)memAlloc(sizeof(field_t));
			sequence_push(&(t->fields),f);

      parseFields(in, f, unnamed_types, named_types, 1);
      
      //next field
      getLAnglebracket(in);
      token = getToken(in);	
    }
    if(strcmp("/",token))in->error(in,"not a valid structure definition");
    token = getName(in);
    if(strcmp("struct",token)!=0)
      in->error(in,"not a valid structure definition");
    getRAnglebracket(in); //</struct>
  }
  else if(strcmp(token,"union") == 0) {
    t->type = UNION;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getRAnglebracket(in); //<union>

    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(strcmp("field",token) == 0){
			f = (field_t *)memAlloc(sizeof(field_t));
			sequence_push(&(t->fields),f);
      parseFields(in, f, unnamed_types, named_types, 1);
      
      //next field
      getLAnglebracket(in);
      token = getToken(in);	
    }
    if(strcmp("/",token))in->error(in,"not a valid union definition");
    token = getName(in);
    if(strcmp("union",token)!=0)
      in->error(in,"not a valid union definition");        
    getRAnglebracket(in); //</union>
  }
  else if(strcmp(token,"array") == 0) {
    t->type = ARRAY;
    sequence_init(&(t->fields));
    getTypeAttributes(in, t, unnamed_types, named_types);
    if(t->size == 0) in->error(in, "Array has empty size");
    getForwardslash(in);
    getRAnglebracket(in); //<array size=n>

    //getLAnglebracket(in); //<subtype> 
		/* subfield */
		f = (field_t *)memAlloc(sizeof(field_t));
		sequence_push(&(t->fields),f);
    parseFields(in, f, unnamed_types, named_types, 0);

    //getLAnglebracket(in); //<type struct> 
    //t->nested_type = parseType(in, NULL, unnamed_types, named_types);

    getLAnglebracket(in); //</array>
    getForwardslash(in);
    token = getName(in);
    if(strcmp("array",token))in->error(in,"not a valid array definition");
    getRAnglebracket(in);  //</array>
  }
  else if(strcmp(token,"sequence") == 0) {
    t->type = SEQUENCE;
    sequence_init(&(t->fields));
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); //<sequence>

    //getLAnglebracket(in); //<sequence size type> 
		/* subfield */
		f = (field_t *)memAlloc(sizeof(field_t));
		sequence_push(&(t->fields),f);
    parseFields(in, f, unnamed_types, named_types, 0);

    //getLAnglebracket(in); //<subtype> 
		/* subfield */
		f = (field_t *)memAlloc(sizeof(field_t));
		sequence_push(&(t->fields),f);
    parseFields(in, f, unnamed_types, named_types, 0);

    //getLAnglebracket(in); //<type sequence> 
    //t->length_type = parseType(in, NULL, unnamed_types, named_types);

    //getLAnglebracket(in); //<type sequence> 

    //t->nested_type = parseType(in, NULL, unnamed_types, named_types);

    if(t->fields.position < 1) in->error(in, "Sequence has no length type");
    if(t->fields.position < 2) in->error(in, "Sequence has no subtype");
		switch(((field_t*)t->fields.array[0])->type->type) {
			case UINT_FIXED :
			case UCHAR :
			case USHORT :
			case UINT :
			case ULONG :
			case SIZE_T :
			case OFF_T :
				break;
			default:
				in->error(in, "Wrong length type for sequence");
		}

    getLAnglebracket(in); //</sequence>
    getForwardslash(in);
    token = getName(in);
    if(strcmp("sequence",token))in->error(in,"not a valid sequence definition");
    getRAnglebracket(in); //</sequence>
  }
  else if(strcmp(token,"enum") == 0) {
    char * str;
    int value = -1;

    t->type = ENUM;
    sequence_init(&(t->labels));
    sequence_init(&(t->labels_values));
    sequence_init(&(t->labels_description));
		t->already_printed = 0;
    getTypeAttributes(in, t, unnamed_types, named_types);
    //if(t->size == 0) in->error(in, "Sequence has empty size");
		//Mathieu : we fix enum size to target int size. GCC is always like this.
		//fox copy optimisation.
    if(t->size != 0) in->error(in, "Enum has fixed size of target int.");
		t->size = 0;
    getRAnglebracket(in);

    //<label name=label1 value=n/>
    getLAnglebracket(in);
    token = getToken(in); //"label" or "/"
    while(strcmp("label",token) == 0){
      int *label_value = malloc(sizeof(int));
      
      str   = allocAndCopy(getNameAttribute(in));
      token = getValueStrAttribute(in);
      
     	sequence_push(&(t->labels),str);

      if(token) value = strtol(token, NULL, 0);
      else value++;

      *label_value = value;
      sequence_push(&(t->labels_values), label_value);

      getForwardslash(in);
      getRAnglebracket(in);
      
      //read description if any. May be NULL.
      str = allocAndCopy(getDescription(in));
			sequence_push(&(t->labels_description),str);
			      
      //next label definition
      getLAnglebracket(in);
      token = getToken(in); //"label" or "/"      
    }
    if(strcmp("/",token))in->error(in, "not a valid enum definition");
    token = getName(in);
    if(strcmp("enum",token))in->error(in, "not a valid enum definition");
      getRAnglebracket(in); //</label>
  }
  else if(strcmp(token,"int_fixed") == 0) {
    t->type = INT_FIXED;
    getTypeAttributes(in, t, unnamed_types, named_types);
    if(t->size == 0) in->error(in, "int has empty size");
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"uint_fixed") == 0) {
    t->type = UINT_FIXED;
    getTypeAttributes(in, t, unnamed_types, named_types);
    if(t->size == 0) in->error(in, "uint has empty size");
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"char") == 0) {
    t->type = CHAR;
    getTypeAttributes(in, t, unnamed_types, named_types);
		t->size = 1;
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"uchar") == 0) {
    t->type = UCHAR;
    getTypeAttributes(in, t, unnamed_types, named_types);
		t->size = 1;
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"short") == 0) {
    t->type = SHORT;
    getTypeAttributes(in, t, unnamed_types, named_types);
		t->size = 2;
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"ushort") == 0) {
    t->type = USHORT;
    getTypeAttributes(in, t, unnamed_types, named_types);
		t->size = 2;
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"int") == 0) {
    t->type = INT;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"uint") == 0) {
    t->type = UINT;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }

  else if(strcmp(token,"pointer") == 0) {
    t->type = POINTER;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"long") == 0) {
    t->type = LONG;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"ulong") == 0) {
    t->type = ULONG;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"size_t") == 0) {
    t->type = SIZE_T;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"ssize_t") == 0) {
    t->type = SSIZE_T;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"off_t") == 0) {
    t->type = OFF_T;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"float") == 0) {
    t->type = FLOAT;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"string") == 0) {
    t->type = STRING;
    getTypeAttributes(in, t, unnamed_types, named_types);
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"typeref") == 0){
    // Must be a named type
		free(t);
		sequence_pop(unnamed_types);
		token = getNameAttribute(in);
		t = find_named_type(token, named_types);
		if(t == NULL) in->error(in,"Named referred to must be pre-declared.");
		getForwardslash(in);  //<typeref name=type_name/>
		getRAnglebracket(in);
		return t;
  }else in->error(in,"not a valid type");

  return t;
}    

/*****************************************************************************
 *Function name
 *    find_named_type     : find a named type from hash table 
 *Input params 
 *    name                : type name          
 *    named_types         : array of named types
 *Return values  
 *    type_descriptor *   : a type descriptor                       
 *****************************************************************************/

type_descriptor_t * find_named_type(char *name, table_t * named_types)
{ 
  type_descriptor_t *t;

  t = table_find(named_types,name);

  return t;
}

type_descriptor_t * create_named_type(char *name, table_t * named_types)
{
  type_descriptor_t *t;

	t = (type_descriptor_t *)memAlloc(sizeof(type_descriptor_t));
	t->type_name = allocAndCopy(name);
	t->type = NONE;
	t->fmt = NULL;
	table_insert(named_types,t->type_name,t);
	//    table_insert(named_types,allocAndCopy(name),t);
	return t;
}

/*****************************************************************************
 *Function name
 *    parseTypeDefinition : get type information from type definition 
 *Input params 
 *    in                  : input file handle          
 *    unnamed_types       : array of unamed types
 *    named_types         : array of named types
 *****************************************************************************/

void parseTypeDefinition(parse_file_t * in, sequence_t * unnamed_types,
			 table_t * named_types)
{
  char *token;
  type_descriptor_t *t;

  token = getNameAttribute(in);
  if(token == NULL) in->error(in, "Type has empty name");
  t = create_named_type(token, named_types);

  if(t->type != NONE) in->error(in,"redefinition of named type");
  getRAnglebracket(in); //<type name=type_name>
  getLAnglebracket(in); //<
  token = getName(in);
  //MD ??if(strcmp("struct",token))in->error(in,"not a valid type definition");
  ungetToken(in);
  parseType(in,t, unnamed_types, named_types);
  
  //</type>
  getLAnglebracket(in);
  getForwardslash(in);
  token = getName(in);
  if(strcmp("type",token))in->error(in,"not a valid type definition");  
  getRAnglebracket(in); //</type>
}

/**************************************************************************
 * Function :
 *    getComa, getName, getNumber, getEqual
 * Description :
 *    Read a token from the input file, check its type, return it scontent.
 *
 * Parameters :
 *    in , input file handle.
 *
 * Return values :
 *    address of token content.
 *
 **************************************************************************/

char *getName(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  // Optional descriptions
	// if(in->type != NAME) in->error(in,"Name token was expected");
  return token;
}

int getNumber(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != NUMBER) in->error(in, "Number token was expected");
  return atoi(token);
}

char *getForwardslash(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  //if(in->type != FORWARDSLASH) in->error(in, "forward slash token was expected");
	/* Mathieu : final / is optional now. */
  if(in->type != FORWARDSLASH) ungetToken(in);

  return token;
}

char *getLAnglebracket(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != LANGLEBRACKET) in->error(in, "Left angle bracket was expected");
  return token;
}

char *getRAnglebracket(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != RANGLEBRACKET) in->error(in, "Right angle bracket was expected");
  return token;
}

char *getQuotedString(parse_file_t * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != QUOTEDSTRING) in->error(in, "quoted string was expected");
  return token;
}

char * getEqual(parse_file_t *in)
{
  char *token;

  token = getToken(in);
  if(in->type != EQUAL) in->error(in, "equal was expected");
  return token;
}

char seekNextChar(parse_file_t *in)
{
  char car;
  while((car = getc(in->fp)) != EOF) {
    if(!isspace(car)){
      ungetc(car,in->fp);
      return car;
    }
  }  
  return EOF;
}

/******************************************************************
 * Function :
 *    getToken, ungetToken
 * Description :
 *    Read a token from the input file and return its type and content.
 *    Line numbers are accounted for and whitespace/comments are skipped.
 *
 * Parameters :
 *    in, input file handle.
 *
 * Return values :
 *    address of token content.
 *
 ******************************************************************/

void ungetToken(parse_file_t * in)
{
  in->unget = 1;
}

char *getToken(parse_file_t * in)
{
  FILE *fp = in->fp;
  char car, car1;
  int pos = 0, escaped;

  if(in->unget == 1) {
    in->unget = 0;
    return in->buffer;
  }

  /* skip whitespace and comments */

  while((car = getc(fp)) != EOF) {
    if(car == '/') {
      car1 = getc(fp); 
      if(car1 == '*') skipComment(in);
      else if(car1 == '/') skipEOL(in);
      else { 
        car1 = ungetc(car1,fp);
        break;
      }
    }
    else if(car == '\n') in->lineno++;
    else if(!isspace(car)) break;
  }

  switch(car) {
    case EOF:
      in->type = ENDFILE;
      break;
    case '/':
      in->type = FORWARDSLASH;
      in->buffer[pos] = car;
      pos++;
      break;
    case '<':
      in->type = LANGLEBRACKET;
      in->buffer[pos] = car;
      pos++;
      break;
    case '>':
      in->type = RANGLEBRACKET;
      in->buffer[pos] = car;
      pos++;
      break;
    case '=':
      in->type = EQUAL;
      in->buffer[pos] = car;
      pos++;
      break;
    case '"':
      escaped = 0;
      while((car = getc(fp)) != EOF && pos < BUFFER_SIZE) {
        if(car == '\\' && escaped == 0) {
	  in->buffer[pos] = car;
	  pos++;
          escaped = 1;
          continue;
        }
        if(car == '"' && escaped == 0) break;
        if(car == '\n' && escaped == 0) {
          in->error(in, "non escaped newline inside quoted string");
        }
        if(car == '\n') in->lineno++;
        in->buffer[pos] = car;
        pos++;
        escaped = 0;
      }
      if(car == EOF) in->error(in,"no ending quotemark");
      if(pos == BUFFER_SIZE) in->error(in, "quoted string token too large");
      in->type = QUOTEDSTRING;
      break;
    default:
      if(isdigit(car)) {
        in->buffer[pos] = car;
        pos++;
        while((car = getc(fp)) != EOF && pos < BUFFER_SIZE) {
          if(!isdigit(car)) {
            ungetc(car,fp);
            break;
          }
          in->buffer[pos] = car;
          pos++;
        }
	if(car == EOF) ungetc(car,fp);
        if(pos == BUFFER_SIZE) in->error(in, "number token too large");
        in->type = NUMBER;
      }    
      else if(isalnum(car) || car == '_' || car == '-') {
        in->buffer[0] = car;
        pos = 1;
        while((car = getc(fp)) != EOF && pos < BUFFER_SIZE) {
          if(!(isalnum(car) || car == '_' || car == '-')) {
            ungetc(car,fp);
            break;
          }
          in->buffer[pos] = car;
          pos++;
        }
	if(car == EOF) ungetc(car,fp);
        if(pos == BUFFER_SIZE) in->error(in, "name token too large");
        in->type = NAME;
      }
      else in->error(in, "invalid character, unrecognized token");
  }
  in->buffer[pos] = 0;
  return in->buffer;
}

void skipComment(parse_file_t * in)
{
  char car;
  while((car = getc(in->fp)) != EOF) {
    if(car == '\n') in->lineno++;
    else if(car == '*') {
      car = getc(in->fp);
      if(car ==EOF) break;
      if(car == '/') return;
      ungetc(car,in->fp);
    }
  }
  if(car == EOF) in->error(in,"comment begining with '/*' has no ending '*/'");
}

void skipEOL(parse_file_t * in)
{
  char car;
  while((car = getc(in->fp)) != EOF) {
    if(car == '\n') {
      ungetc(car,in->fp);
      break;
    }
  }
  if(car == EOF)ungetc(car, in->fp);
}

/*****************************************************************************
 *Function name
 *    checkNamedTypesImplemented : check if all named types have definition
 ****************************************************************************/

void checkNamedTypesImplemented(table_t * named_types)
{
  type_descriptor_t *t;
  int pos;
  char str[256];

  for(pos = 0 ; pos < named_types->values.position; pos++) {
    t = (type_descriptor_t *) named_types->values.array[pos];
    if(t->type == NONE){
      sprintf(str,"named type '%s' has no definition",
          (char*)named_types->keys.array[pos]);
      error_callback(NULL,str);   
    }
  }
}


/*****************************************************************************
 *Function name
 *    generateChecksum  : generate checksum for the facility
 *Input Params
 *    facName           : name of facility
 *Output Params
 *    checksum          : checksum for the facility
 ****************************************************************************/

void generateChecksum(char* facName,
    unsigned int * checksum, sequence_t * events)
{
  unsigned long crc ;
  int pos;
  event_t * ev;
  unsigned int i;

  crc = crc32(facName);
  for(pos = 0; pos < events->position; pos++){
    ev = (event_t *)(events->array[pos]);
    crc = partial_crc32(ev->name, crc);
		for(i = 0; i < ev->fields.position; i++) {
			field_t *f = (field_t*)ev->fields.array[i];
      crc = partial_crc32(f->name, crc);
      crc = getTypeChecksum(crc, f->type);
		}
  }
  *checksum = crc;
}

/*****************************************************************************
 *Function name
 *   getTypeChecksum    : generate checksum by type info
 *Input Params
 *    crc               : checksum generated so far
 *    type              : type descriptor containing type info
 *Return value          
 *    unsigned long     : checksum 
 *****************************************************************************/

unsigned long getTypeChecksum(unsigned long aCrc, type_descriptor_t * type)
{
  unsigned long crc = aCrc;
  char * str = NULL, buf[16];
  int flag = 0, pos;
  field_t * fld;

  switch(type->type){
    case INT_FIXED:
      str = intOutputTypes[getSizeindex(type->size)];
      break;
    case UINT_FIXED:
      str = uintOutputTypes[getSizeindex(type->size)];
      break;
    case POINTER:
      str = allocAndCopy("void *");
			flag = 1;
      break;
    case CHAR:
      str = allocAndCopy("signed char");
			flag = 1;
      break;
    case UCHAR:
      str = allocAndCopy("unsigned char");
			flag = 1;
      break;
    case SHORT:
      str = allocAndCopy("short");
			flag = 1;
      break;
    case USHORT:
      str = allocAndCopy("unsigned short");
			flag = 1;
      break;
    case INT:
      str = allocAndCopy("int");
			flag = 1;
      break;
    case UINT:
      str = allocAndCopy("uint");
			flag = 1;
      break;
    case LONG:
      str = allocAndCopy("long");
			flag = 1;
      break;
    case ULONG:
      str = allocAndCopy("unsigned long");
			flag = 1;
      break;
    case SIZE_T:
      str = allocAndCopy("size_t");
			flag = 1;
      break;
    case SSIZE_T:
      str = allocAndCopy("ssize_t");
			flag = 1;
      break;
    case OFF_T:
      str = allocAndCopy("off_t");
			flag = 1;
      break;
    case FLOAT:
      str = floatOutputTypes[getSizeindex(type->size)];
      break;
    case STRING:
      str = allocAndCopy("string");
      flag = 1;
      break;
    case ENUM:
      //str = appendString("enum ", uintOutputTypes[getSizeindex(type->size)]);
      str = allocAndCopy("enum");
      flag = 1;
      break;
    case ARRAY:
      sprintf(buf,"%zu", type->size);
      str = appendString("array ",buf);
      flag = 1;
      break;
    case SEQUENCE:
      str = allocAndCopy("sequence ");
      flag = 1;
      break;
    case STRUCT:
      str = allocAndCopy("struct");
      flag = 1;
      break;
    case UNION:
      str = allocAndCopy("union");
      flag = 1;
      break;
    default:
      error_callback(NULL, "named type has no definition");
      break;
  }

  crc = partial_crc32(str,crc);
  if(flag) free(str);

  if(type->fmt) crc = partial_crc32(type->fmt,crc);
    
  if(type->type == ARRAY){
    crc = getTypeChecksum(crc,((field_t*)type->fields.array[0])->type);
  } else if(type->type ==SEQUENCE) {
    crc = getTypeChecksum(crc,((field_t*)type->fields.array[0])->type);
    crc = getTypeChecksum(crc,((field_t*)type->fields.array[1])->type);
	} else if(type->type == STRUCT || type->type == UNION){
    for(pos =0; pos < type->fields.position; pos++){
      fld = (field_t *) type->fields.array[pos];
      crc = partial_crc32(fld->name,crc);
      crc = getTypeChecksum(crc, fld->type);
    }    
  }else if(type->type == ENUM){
    for(pos = 0; pos < type->labels.position; pos++)
      crc = partial_crc32((char*)type->labels.array[pos],crc);
  }

  return crc;
}


/* Event type descriptors */
void freeType(type_descriptor_t * tp)
{
  int pos2;
  field_t *f;

  if(tp->fmt != NULL) free(tp->fmt);
  if(tp->type == ENUM) {
    for(pos2 = 0; pos2 < tp->labels.position; pos2++) {
      free(tp->labels.array[pos2]);
    }
    sequence_dispose(&(tp->labels));
    for(pos2 = 0; pos2 < tp->labels_values.position; pos2++) {
      free(tp->labels_values.array[pos2]);
    }
    sequence_dispose(&(tp->labels_values));
  }
  if(tp->type == STRUCT) {
    for(pos2 = 0; pos2 < tp->fields.position; pos2++) {
      f = (field_t *) tp->fields.array[pos2];
      free(f->name);
      free(f->description);
      free(f);
    }
    sequence_dispose(&(tp->fields));
  }
}

void freeNamedType(table_t * t)
{
  int pos;
  type_descriptor_t * td;

  for(pos = 0 ; pos < t->keys.position; pos++) {
    free((char *)t->keys.array[pos]);
    td = (type_descriptor_t*)t->values.array[pos];
    freeType(td);
    free(td);
  }
}

void freeTypes(sequence_t *t) 
{
  int pos;
  type_descriptor_t *tp;

  for(pos = 0 ; pos < t->position; pos++) {
    tp = (type_descriptor_t *)t->array[pos];
    freeType(tp);
    free(tp);
  }
}

void freeEvents(sequence_t *t) 
{
  int pos;
  event_t *ev;

  for(pos = 0 ; pos < t->position; pos++) {
    ev = (event_t *) t->array[pos];
    free(ev->name);
    free(ev->description);
		sequence_dispose(&ev->fields);
    free(ev);
  }

}


/* Extensible array */

void sequence_init(sequence_t *t) 
{
  t->size = 10;
  t->position = 0;
  t->array = (void **)memAlloc(t->size * sizeof(void *));
}

void sequence_dispose(sequence_t *t) 
{
  t->size = 0;
  free(t->array);
  t->array = NULL;
}

void sequence_push(sequence_t *t, void *elem) 
{
  void **tmp;

  if(t->position >= t->size) {
    tmp = t->array;
    t->array = (void **)memAlloc(t->size * 2 * sizeof(void *));
    memcpy(t->array, tmp, t->size * sizeof(void *));
    t->size = t->size * 2;
    free(tmp);
  }
  t->array[t->position] = elem;
  t->position++;
}

void *sequence_pop(sequence_t *t) 
{
	if(t->position == 0) printf("Error : trying to pop an empty sequence");
  return t->array[--t->position];
}


/* Hash table API, implementation is just linear search for now */

void table_init(table_t *t) 
{
  sequence_init(&(t->keys));
  sequence_init(&(t->values));
}

void table_dispose(table_t *t) 
{
  sequence_dispose(&(t->keys));
  sequence_dispose(&(t->values));
}

void table_insert(table_t *t, char *key, void *value) 
{
  sequence_push(&(t->keys),key);
  sequence_push(&(t->values),value);
}

void *table_find(table_t *t, char *key) 
{
  int pos;
  for(pos = 0 ; pos < t->keys.position; pos++) {
    if(strcmp((char *)key,(char *)t->keys.array[pos]) == 0)
      return(t->values.array[pos]);
  }
  return NULL;
}

void table_insert_int(table_t *t, int *key, void *value)
{
  sequence_push(&(t->keys),key);
  sequence_push(&(t->values),value);
}

void *table_find_int(table_t *t, int *key)
{
  int pos;
  for(pos = 0 ; pos < t->keys.position; pos++) {
    if(*key == *(int *)t->keys.array[pos])
      return(t->values.array[pos]);
  }
  return NULL;
}


/* Concatenate strings */

char *appendString(char *s, char *suffix) 
{
  char *tmp;
  if(suffix == NULL) return s;

  tmp = (char *)memAlloc(strlen(s) + strlen(suffix) + 1);
  strcpy(tmp,s);
  strcat(tmp,suffix);  
  return tmp;
}
