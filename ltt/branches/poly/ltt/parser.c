/*

parser.c: Generate helper declarations and functions to trace events
  from an event description file.

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


#include "parser.h"


/*****************************************************************************
 *Function name
 *    getSize    : translate from string to integer
 *Input params 
 *    in         : input file handle
 *Return values  
 *    size                           
 *****************************************************************************/

int getSize(parse_file *in)
{
  char *token;

  token = getToken(in);
  if(in->type == NUMBER) {
    if(strcmp(token,"1") == 0) return 0;
    else if(strcmp(token,"2") == 0) return 1;
    else if(strcmp(token,"4") == 0) return 2;
    else if(strcmp(token,"8") == 0) return 3;
  }
  else if(in->type == NAME) {
    if(strcmp(token,"short") == 0) return 4;
    else if(strcmp(token,"medium") == 0) return 5;
    else if(strcmp(token,"long") == 0) return 6;
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

void error_callback(parse_file *in, char *msg)
{
  if(in)
    printf("Error in file %s, line %d: %s\n", in->name, in->lineno, msg);
  else
    printf("%s\n",msg);
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
 *    getNameAttribute,getFormatAttribute,getSizeAttribute,getValueAttribute 
 *    getValueStrAttribute
 * Description :
 *    Read the attribute from the input file.
 *
 * Parameters :
 *    in , input file handle.
 *
 * Return values :
 *    address of the attribute.
 *
 **************************************************************************/

char * getNameAttribute(parse_file *in)
{
  char * token, car;
  token = getName(in);
  if(strcmp("name",token))in->error(in,"name was expected");
  getEqual(in);
  
  car = seekNextChar(in);
  if(car == EOF)in->error(in,"name was expected");
  else if(car == '\"')token = getQuotedString(in);
  else token = getName(in);
  return token;
}

char * getFormatAttribute(parse_file *in)
{
  char * token;

  //format is an option
  token = getToken(in); 
  if(strcmp("/",token) == 0 || strcmp(">",token) == 0){
    ungetToken(in);
    return NULL;
  }

  if(strcmp("format",token))in->error(in,"format was expected");
  getEqual(in);
  token = getQuotedString(in);
  return token;
}

int getSizeAttribute(parse_file *in)
{
  char * token;
  getName(in);
  getEqual(in);

  return getSize(in);
}

int getValueAttribute(parse_file *in)
{
  char * token;
  getName(in);
  getEqual(in);
  
  return getNumber(in);
}

//for <label name=label_name value=n/>, value is an option
char * getValueStrAttribute(parse_file *in)
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

char * getDescription(parse_file *in)
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

void parseFacility(parse_file *in, facility * fac)
{
  char * token;
  event *ev;
  
  fac->name = allocAndCopy(getNameAttribute(in));    
  getRAnglebracket(in);    
  
  fac->description = allocAndCopy(getDescription(in));
  
  while(1){
    getLAnglebracket(in);    

    token = getToken(in);
    if(in->type == ENDFILE)
      in->error(in,"the definition of the facility is not finished");

    if(strcmp("event",token) == 0){
      ev = (event*) memAlloc(sizeof(event));
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

void parseEvent(parse_file *in, event * ev, sequence * unnamed_types, 
		table * named_types) 
{
  char *token;
  type_descriptor *t;

  //<event name=eventtype_name>
  ev->name = allocAndCopy(getNameAttribute(in));
  getRAnglebracket(in);  

  //<description>...</descriptio>
  ev->description = allocAndCopy(getDescription(in)); 
  
  //event can have STRUCT, TYPEREF or NOTHING
  getLAnglebracket(in);

  token = getToken(in);
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

  token = getName(in);
  if(strcmp("event",token))in->error(in,"not an event definition");
  getRAnglebracket(in);  //</event>
}

/*****************************************************************************
 *Function name
 *    parseField    : get field infomation from buffer 
 *Input params 
 *    in            : input file handle
 *    t             : type descriptor
 *    unnamed_types : array of unamed types
 *    named_types   : array of named types
 ****************************************************************************/

void parseFields(parse_file *in, type_descriptor *t, sequence * unnamed_types,
		 table * named_types) 
{
  char * token;
  field *f;

  f = (field *)memAlloc(sizeof(field));
  sequence_push(&(t->fields),f);

  //<field name=field_name> <description> <type> </field>
  f->name = allocAndCopy(getNameAttribute(in)); 
  getRAnglebracket(in);

  f->description = allocAndCopy(getDescription(in));

  //<int size=...>
  getLAnglebracket(in);
  f->type = parseType(in,NULL, unnamed_types, named_types);

  getLAnglebracket(in);
  getForwardslash(in);
  token = getName(in);
  if(strcmp("field",token))in->error(in,"not a valid field definition");
  getRAnglebracket(in); //</field>
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

type_descriptor *parseType(parse_file *in, type_descriptor *inType, 
			   sequence * unnamed_types, table * named_types) 
{
  char *token;
  type_descriptor *t;

  if(inType == NULL) {
    t = (type_descriptor *) memAlloc(sizeof(type_descriptor));
    t->type_name = NULL;
    t->type = NONE;
    t->fmt = NULL;
    sequence_push(unnamed_types,t);
  }
  else t = inType;

  token = getName(in);

  if(strcmp(token,"struct") == 0) {
    t->type = STRUCT;
    getRAnglebracket(in); //<struct>
    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(strcmp("field",token) == 0){
      parseFields(in,t, unnamed_types, named_types);
      
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
    t->size = getSizeAttribute(in);
    getRAnglebracket(in); //<union typecodesize=isize>

    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(strcmp("field",token) == 0){
      parseFields(in,t, unnamed_types, named_types);
      
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
    t->size = getValueAttribute(in);
    getRAnglebracket(in); //<array size=n>

    getLAnglebracket(in); //<type struct> 
    t->nested_type = parseType(in,NULL, unnamed_types, named_types);

    getLAnglebracket(in); //</array>
    getForwardslash(in);
    token = getName(in);
    if(strcmp("array",token))in->error(in,"not a valid array definition");
    getRAnglebracket(in);  //</array>
  }
  else if(strcmp(token,"sequence") == 0) {
    t->type = SEQUENCE;
    t->size = getSizeAttribute(in);
    getRAnglebracket(in); //<array lengthsize=isize>

    getLAnglebracket(in); //<type struct> 
    t->nested_type = parseType(in,NULL, unnamed_types, named_types);

    getLAnglebracket(in); //</sequence>
    getForwardslash(in);
    token = getName(in);
    if(strcmp("sequence",token))in->error(in,"not a valid sequence definition");
    getRAnglebracket(in); //</sequence>
  }
  else if(strcmp(token,"enum") == 0) {
    char * str, *str1;
    t->type = ENUM;
    sequence_init(&(t->labels));
    t->size = getSizeAttribute(in);
    t->fmt = allocAndCopy(getFormatAttribute(in));
    getRAnglebracket(in);

    //<label name=label1 value=n/>
    getLAnglebracket(in);
    token = getToken(in); //"label" or "/"
    while(strcmp("label",token) == 0){
      str   = allocAndCopy(getNameAttribute(in));      
      token = getValueStrAttribute(in);
      if(token){
	str1 = appendString(str,"=");
	free(str);
	str = appendString(str1,token);
	free(str1);
	sequence_push(&(t->labels),allocAndCopy(str));
	free(str);
      }else
	sequence_push(&(t->labels),allocAndCopy(str));

      getForwardslash(in);
      getRAnglebracket(in);
      
      //next label definition
      getLAnglebracket(in);
      token = getToken(in); //"label" or "/"      
    }
    if(strcmp("/",token))in->error(in, "not a valid enum definition");
    token = getName(in);
    if(strcmp("enum",token))in->error(in, "not a valid enum definition");
    getRAnglebracket(in); //</label>
  }
  else if(strcmp(token,"int") == 0) {
    t->type = INT;
    t->size = getSizeAttribute(in);
    t->fmt  = allocAndCopy(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"uint") == 0) {
    t->type = UINT;
    t->size = getSizeAttribute(in);
    t->fmt  = allocAndCopy(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"float") == 0) {
    t->type = FLOAT;
    t->size = getSizeAttribute(in);
    t->fmt  = allocAndCopy(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"string") == 0) {
    t->type = STRING;
    t->fmt  = allocAndCopy(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(strcmp(token,"typeref") == 0){
    // Must be a named type
    if(inType != NULL) 
      in->error(in,"Named type cannot refer to a named type");
    else {
      free(t);
      sequence_pop(unnamed_types);
      token = getNameAttribute(in);
      t = find_named_type(token, named_types);
      getForwardslash(in);  //<typeref name=type_name/>
      getRAnglebracket(in);
      return t;
    }
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

type_descriptor * find_named_type(char *name, table * named_types)
{ 
  type_descriptor *t;

  t = table_find(named_types,name);
  if(t == NULL) {
    t = (type_descriptor *)memAlloc(sizeof(type_descriptor));
    t->type_name = allocAndCopy(name);
    t->type = NONE;
    t->fmt = NULL;
    table_insert(named_types,allocAndCopy(name),t);
  }
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

void parseTypeDefinition(parse_file * in, sequence * unnamed_types,
			 table * named_types)
{
  char *token;
  type_descriptor *t;

  token = getNameAttribute(in);
  t = find_named_type(token, named_types);

  if(t->type != NONE) in->error(in,"redefinition of named type");
  getRAnglebracket(in); //<type name=type_name>
  getLAnglebracket(in); //<struct>
  token = getName(in);
  if(strcmp("struct",token))in->error(in,"not a valid type definition");
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

char *getName(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != NAME) in->error(in,"Name token was expected");
  return token;
}

int getNumber(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != NUMBER) in->error(in, "Number token was expected");
  return atoi(token);
}

char *getForwardslash(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != FORWARDSLASH) in->error(in, "forward slash token was expected");
  return token;
}

char *getLAnglebracket(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != LANGLEBRACKET) in->error(in, "Left angle bracket was expected");
  return token;
}

char *getRAnglebracket(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != RANGLEBRACKET) in->error(in, "Right angle bracket was expected");
  return token;
}

char *getQuotedString(parse_file * in) 
{
  char *token;

  token = getToken(in);
  if(in->type != QUOTEDSTRING) in->error(in, "quoted string was expected");
  return token;
}

char * getEqual(parse_file *in)
{
  char *token;

  token = getToken(in);
  if(in->type != EQUAL) in->error(in, "equal was expected");
  return token;
}

char seekNextChar(parse_file *in)
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

void ungetToken(parse_file * in)
{
  in->unget = 1;
}

char *getToken(parse_file * in)
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
      else if(isalpha(car)) {
        in->buffer[0] = car;
        pos = 1;
        while((car = getc(fp)) != EOF && pos < BUFFER_SIZE) {
          if(!isalnum(car)) {
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

void skipComment(parse_file * in)
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

void skipEOL(parse_file * in)
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

int isalpha(char c)
{
  int i,j;
  if(c == '_')return 1;
  i = c - 'a';
  j = c - 'A';
  if((i>=0 && i<26) || (j>=0 && j<26)) return 1;
  return 0;
}

int isalnum(char c)
{
  return (isalpha(c) || isdigit(c));
}

/*****************************************************************************
 *Function name
 *    checkNamedTypesImplemented : check if all named types have definition
 ****************************************************************************/

void checkNamedTypesImplemented(table * named_types)
{
  type_descriptor *t;
  int pos;
  char str[256];

  for(pos = 0 ; pos < named_types->values.position; pos++) {
    t = (type_descriptor *) named_types->values.array[pos];
    if(t->type == NONE){
      sprintf(str,"named type '%s' has no definition",(char*)named_types->keys.array[pos]);
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

void generateChecksum( char* facName, unsigned long * checksum, sequence * events)
{
  unsigned long crc ;
  int pos;
  event * ev;
  char str[256];

  crc = crc32(facName);
  for(pos = 0; pos < events->position; pos++){
    ev = (event *)(events->array[pos]);
    crc = partial_crc32(ev->name,crc);    
    if(!ev->type) continue; //event without type
    if(ev->type->type != STRUCT){
      sprintf(str,"event '%s' has a type other than STRUCT",ev->name);
      error_callback(NULL, str);
    }
    crc = getTypeChecksum(crc, ev->type);
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

unsigned long getTypeChecksum(unsigned long aCrc, type_descriptor * type)
{
  unsigned long crc = aCrc;
  char * str = NULL, buf[16];
  int flag = 0, pos;
  field * fld;

  switch(type->type){
    case INT:
      str = intOutputTypes[type->size];
      break;
    case UINT:
      str = uintOutputTypes[type->size];
      break;
    case FLOAT:
      str = floatOutputTypes[type->size];
      break;
    case STRING:
      str = allocAndCopy("string");
      flag = 1;
      break;
    case ENUM:
      str = appendString("enum ", uintOutputTypes[type->size]);
      flag = 1;
      break;
    case ARRAY:
      sprintf(buf,"%d\0",type->size);
      str = appendString("array ",buf);
      flag = 1;
      break;
    case SEQUENCE:
      sprintf(buf,"%d\0",type->size);
      str = appendString("sequence ",buf);
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
    
  if(type->type == ARRAY || type->type == SEQUENCE){
    crc = getTypeChecksum(crc,type->nested_type);
  }else if(type->type == STRUCT || type->type == UNION){
    for(pos =0; pos < type->fields.position; pos++){
      fld = (field *) type->fields.array[pos];
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
void freeType(type_descriptor * tp)
{
  int pos2;
  field *f;

  if(tp->fmt != NULL) free(tp->fmt);
  if(tp->type == ENUM) {
    for(pos2 = 0; pos2 < tp->labels.position; pos2++) {
      free(tp->labels.array[pos2]);
    }
    sequence_dispose(&(tp->labels));
  }
  if(tp->type == STRUCT) {
    for(pos2 = 0; pos2 < tp->fields.position; pos2++) {
      f = (field *) tp->fields.array[pos2];
      free(f->name);
      free(f->description);
      free(f);
    }
    sequence_dispose(&(tp->fields));
  }
}

void freeNamedType(table * t)
{
  int pos;
  type_descriptor * td;

  for(pos = 0 ; pos < t->keys.position; pos++) {
    free((char *)t->keys.array[pos]);
    td = (type_descriptor*)t->values.array[pos];
    freeType(td);
    free(td);
  }
}

void freeTypes(sequence *t) 
{
  int pos, pos2;
  type_descriptor *tp;
  field *f;

  for(pos = 0 ; pos < t->position; pos++) {
    tp = (type_descriptor *)t->array[pos];
    freeType(tp);
    free(tp);
  }
}

void freeEvents(sequence *t) 
{
  int pos;
  event *ev;

  for(pos = 0 ; pos < t->position; pos++) {
    ev = (event *) t->array[pos];
    free(ev->name);
    free(ev->description);
    free(ev);
  }

}


/* Extensible array */

void sequence_init(sequence *t) 
{
  t->size = 10;
  t->position = 0;
  t->array = (void **)memAlloc(t->size * sizeof(void *));
}

void sequence_dispose(sequence *t) 
{
  t->size = 0;
  free(t->array);
  t->array = NULL;
}

void sequence_push(sequence *t, void *elem) 
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

void *sequence_pop(sequence *t) 
{
  return t->array[t->position--];
}


/* Hash table API, implementation is just linear search for now */

void table_init(table *t) 
{
  sequence_init(&(t->keys));
  sequence_init(&(t->values));
}

void table_dispose(table *t) 
{
  sequence_dispose(&(t->keys));
  sequence_dispose(&(t->values));
}

void table_insert(table *t, char *key, void *value) 
{
  sequence_push(&(t->keys),key);
  sequence_push(&(t->values),value);
}

void *table_find(table *t, char *key) 
{
  int pos;
  for(pos = 0 ; pos < t->keys.position; pos++) {
    if(strcmp((char *)key,(char *)t->keys.array[pos]) == 0)
      return(t->values.array[pos]);
  }
  return NULL;
}

void table_insert_int(table *t, int *key, void *value)
{
  sequence_push(&(t->keys),key);
  sequence_push(&(t->values),value);
}

void *table_find_int(table *t, int *key)
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


