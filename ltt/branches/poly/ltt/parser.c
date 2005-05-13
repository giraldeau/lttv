/*

parser.c: Generate helper declarations and functions to trace events
  from an event description file.

Copyright (C) 2002, Xianxiu Yang
Copyright (C) 2002, Michel Dagenais
Copyright (C) 2005, Mathieu Desnoyers

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
#include <ctype.h>
#include <linux/errno.h>
#include <glib.h>


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
  gchar *token;

  token = getToken(in);
  if(in->type == NUMBER) {
    if(g_ascii_strcasecmp(token,"1") == 0) return 0;
    else if(g_ascii_strcasecmp(token,"2") == 0) return 1;
    else if(g_ascii_strcasecmp(token,"4") == 0) return 2;
    else if(g_ascii_strcasecmp(token,"8") == 0) return 3;
  }
  else if(in->type == NAME) {
    if(g_ascii_strcasecmp(token,"short") == 0) return 4;
    else if(g_ascii_strcasecmp(token,"medium") == 0) return 5;
    else if(g_ascii_strcasecmp(token,"long") == 0) return 6;
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
    g_printf("Error in file %s, line %d: %s\n", in->name, in->lineno, msg);
  else
    printf("%s\n",msg);
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

gchar * getNameAttribute(parse_file *in)
{
  gchar * token;
  gunichar car;
  GIOStatus status;
  
  token = getName(in);
  if(g_ascii_strcasecmp("name",token))in->error(in,"name was expected");
  getEqual(in);
  
  status = seekNextChar(in, &car);
  if(status == G_IO_STATUS_EOF || status == G_IO_STATUS_ERROR)
    in->error(in,"name was expected");
  else if(car == '\"') token = getQuotedString(in);
  else token = getName(in);
  return token;
}

char * getFormatAttribute(parse_file *in)
{
  char * token;

  //format is an option
  token = getToken(in); 
  if(g_ascii_strcasecmp("/",token) == 0 || g_ascii_strcasecmp(">",token) == 0){
    ungetToken(in);
    return NULL;
  }

  if(g_ascii_strcasecmp("format",token))in->error(in,"format was expected");
  getEqual(in);
  token = getQuotedString(in);
  return token;
}

int getSizeAttribute(parse_file *in)
{
  /* skip name and equal */
  getName(in);
  getEqual(in);

  return getSize(in);
}

int getValueAttribute(parse_file *in)
{
  /* skip name and equal */
  getName(in);
  getEqual(in);
  
  return getNumber(in);
}

//for <label name=label_name value=n/>, value is an option
char * getValueStrAttribute(parse_file *in)
{
  char * token;

  token = getToken(in); 
  if(g_ascii_strcasecmp("/",token) == 0){
    ungetToken(in);
    return NULL;
  }
  
  if(g_ascii_strcasecmp("value",token))in->error(in,"value was expected");
  getEqual(in);
  token = getToken(in);
  if(in->type != NUMBER) in->error(in,"number was expected");
  return token;  
}

char * getDescription(parse_file *in)
{
  gint64 pos;
  gchar * token, *str;
  gunichar car;
  GError * error = NULL;

  pos = in->pos;

  getLAnglebracket(in);
  token = getName(in);
  if(g_ascii_strcasecmp("description",token)){
    g_io_channel_seek_position(in->channel, pos-(in->pos), G_SEEK_CUR, &error);
    if(error != NULL) {
      g_warning("Can not seek file: \n%s\n", error->message);
      g_error_free(error);
    } else in->pos = pos;

    return NULL;
  }
  
  getRAnglebracket(in);

  pos = 0;
  while((g_io_channel_read_unichar(in->channel, &car, &error))
            != G_IO_STATUS_EOF) {
    if(error != NULL) {
      g_warning("Can not seek file: \n%s\n", error->message);
      g_error_free(error);
    } else in->pos++;

    if(car == '<') break;
    if(car == '\0') continue;
    in->buffer[pos] = car;
    pos++;
  }
  if(car == EOF)in->error(in,"not a valid description");
  in->buffer[pos] = '\0';

  str = g_strdup(in->buffer);

  getForwardslash(in);
  token = getName(in);
  if(g_ascii_strcasecmp("description", token))in->error(in,"not a valid description");
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

void parseFacility(parse_file *in, facility_t * fac)
{
  char * token;
  event_t *ev;
  
  fac->name = g_strdup(getNameAttribute(in));    
  getRAnglebracket(in);    
  
  fac->description = getDescription(in);
  
  while(1){
    getLAnglebracket(in);    

    token = getToken(in);
    if(in->type == ENDFILE)
      in->error(in,"the definition of the facility is not finished");

    if(g_ascii_strcasecmp("event",token) == 0){
      ev = (event_t*) g_new(event_t,1);
      sequence_push(&(fac->events),ev);
      parseEvent(in,ev, &(fac->unnamed_types), &(fac->named_types));    
    }else if(g_ascii_strcasecmp("type",token) == 0){
      parseTypeDefinition(in, &(fac->unnamed_types), &(fac->named_types));
    }else if(in->type == FORWARDSLASH){
      break;
    }else in->error(in,"event or type token expected\n");
  }

  token = getName(in);
  if(g_ascii_strcasecmp("facility",token)) in->error(in,"not the end of the facility");
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

void parseEvent(parse_file *in, event_t * ev, sequence * unnamed_types, 
		table * named_types) 
{
  char *token;

  //<event name=eventtype_name>
  ev->name = g_strdup(getNameAttribute(in));
  getRAnglebracket(in);  

  //<description>...</descriptio>
  ev->description = getDescription(in); 
  
  //event can have STRUCT, TYPEREF or NOTHING
  getLAnglebracket(in);

  token = getToken(in);
  if(in->type == FORWARDSLASH){ //</event> NOTHING
    ev->type = NULL;
  }else if(in->type == NAME){
    if(g_ascii_strcasecmp("struct",token)==0 || g_ascii_strcasecmp("typeref",token)==0){
      ungetToken(in);
      ev->type = parseType(in,NULL, unnamed_types, named_types);
      if(ev->type->type != STRUCT && ev->type->type != NONE) 
	in->error(in,"type must be a struct");     
    }else in->error(in, "not a valid type");

    getLAnglebracket(in);
    getForwardslash(in);    
  }else in->error(in,"not a struct type");

  token = getName(in);
  if(g_ascii_strcasecmp("event",token))in->error(in,"not an event definition");
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
  type_fields *f;

  f = g_new(type_fields,1);
  sequence_push(&(t->fields),f);

  //<field name=field_name> <description> <type> </field>
  f->name = g_strdup(getNameAttribute(in)); 
  getRAnglebracket(in);

  f->description = getDescription(in);

  //<int size=...>
  getLAnglebracket(in);
  f->type = parseType(in,NULL, unnamed_types, named_types);

  getLAnglebracket(in);
  getForwardslash(in);
  token = getName(in);
  if(g_ascii_strcasecmp("field",token))in->error(in,"not a valid field definition");
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
    t = g_new(type_descriptor,1);
    t->type_name = NULL;
    t->type = NONE;
    t->fmt = NULL;
    sequence_push(unnamed_types,t);
  }
  else t = inType;

  token = getName(in);

  if(g_ascii_strcasecmp(token,"struct") == 0) {
    t->type = STRUCT;
    getRAnglebracket(in); //<struct>
    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(g_ascii_strcasecmp("field",token) == 0){
      parseFields(in,t, unnamed_types, named_types);
      
      //next field
      getLAnglebracket(in);
      token = getToken(in);	
    }
    if(g_ascii_strcasecmp("/",token))in->error(in,"not a valid structure definition");
    token = getName(in);
    if(g_ascii_strcasecmp("struct",token)!=0)
      in->error(in,"not a valid structure definition");
    getRAnglebracket(in); //</struct>
  }
  else if(g_ascii_strcasecmp(token,"union") == 0) {
    t->type = UNION;
    t->size = getSizeAttribute(in);
    getRAnglebracket(in); //<union typecodesize=isize>

    getLAnglebracket(in); //<field name=..>
    token = getToken(in);
    sequence_init(&(t->fields));
    while(g_ascii_strcasecmp("field",token) == 0){
      parseFields(in,t, unnamed_types, named_types);
      
      //next field
      getLAnglebracket(in);
      token = getToken(in);	
    }
    if(g_ascii_strcasecmp("/",token))in->error(in,"not a valid union definition");
    token = getName(in);
    if(g_ascii_strcasecmp("union",token)!=0)
      in->error(in,"not a valid union definition");        
    getRAnglebracket(in); //</union>
  }
  else if(g_ascii_strcasecmp(token,"array") == 0) {
    t->type = ARRAY;
    t->size = getValueAttribute(in);
    getRAnglebracket(in); //<array size=n>

    getLAnglebracket(in); //<type struct> 
    t->nested_type = parseType(in,NULL, unnamed_types, named_types);

    getLAnglebracket(in); //</array>
    getForwardslash(in);
    token = getName(in);
    if(g_ascii_strcasecmp("array",token))in->error(in,"not a valid array definition");
    getRAnglebracket(in);  //</array>
  }
  else if(g_ascii_strcasecmp(token,"sequence") == 0) {
    t->type = SEQUENCE;
    t->size = getSizeAttribute(in);
    getRAnglebracket(in); //<array lengthsize=isize>

    getLAnglebracket(in); //<type struct> 
    t->nested_type = parseType(in,NULL, unnamed_types, named_types);

    getLAnglebracket(in); //</sequence>
    getForwardslash(in);
    token = getName(in);
    if(g_ascii_strcasecmp("sequence",token))in->error(in,"not a valid sequence definition");
    getRAnglebracket(in); //</sequence>
  }
  else if(g_ascii_strcasecmp(token,"enum") == 0) {
    char * str, *str1;
    t->type = ENUM;
    sequence_init(&(t->labels));
    t->size = getSizeAttribute(in);
    t->fmt = g_strdup(getFormatAttribute(in));
    getRAnglebracket(in);

    //<label name=label1 value=n/>
    getLAnglebracket(in);
    token = getToken(in); //"label" or "/"
    while(g_ascii_strcasecmp("label",token) == 0){
      str1   = g_strdup(getNameAttribute(in));      
      token = getValueStrAttribute(in);
      if(token){
	      str = g_strconcat(str1,"=",token,NULL);
      	g_free(str1);
      	sequence_push(&(t->labels),str);
      }else
      	sequence_push(&(t->labels),str1);

      getForwardslash(in);
      getRAnglebracket(in);
      
      //next label definition
      getLAnglebracket(in);
      token = getToken(in); //"label" or "/"      
    }
    if(g_ascii_strcasecmp("/",token))in->error(in, "not a valid enum definition");
    token = getName(in);
    if(g_ascii_strcasecmp("enum",token))in->error(in, "not a valid enum definition");
    getRAnglebracket(in); //</label>
  }
  else if(g_ascii_strcasecmp(token,"int") == 0) {
    t->type = INT;
    t->size = getSizeAttribute(in);
    t->fmt  = g_strdup(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(g_ascii_strcasecmp(token,"uint") == 0) {
    t->type = UINT;
    t->size = getSizeAttribute(in);
    t->fmt  = g_strdup(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(g_ascii_strcasecmp(token,"float") == 0) {
    t->type = FLOAT;
    t->size = getSizeAttribute(in);
    t->fmt  = g_strdup(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(g_ascii_strcasecmp(token,"string") == 0) {
    t->type = STRING;
    t->fmt  = g_strdup(getFormatAttribute(in));
    getForwardslash(in);
    getRAnglebracket(in); 
  }
  else if(g_ascii_strcasecmp(token,"typeref") == 0){
    // Must be a named type
    if(inType != NULL) 
      in->error(in,"Named type cannot refer to a named type");
    else {
      g_free(t);
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

type_descriptor * find_named_type(gchar *name, table * named_types)
{ 
  type_descriptor *t;

  t = table_find(named_types,name);
  if(t == NULL) {
    t = g_new(type_descriptor,1);
    t->type_name = g_strdup(name);
    t->type = NONE;
    t->fmt = NULL;
    table_insert(named_types,t->type_name,t);
    //    table_insert(named_types,g_strdup(name),t);
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
  if(g_ascii_strcasecmp("struct",token))in->error(in,"not a valid type definition");
  ungetToken(in);
  parseType(in,t, unnamed_types, named_types);
  
  //</type>
  getLAnglebracket(in);
  getForwardslash(in);
  token = getName(in);
  if(g_ascii_strcasecmp("type",token))in->error(in,"not a valid type definition");  
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

gunichar seekNextChar(parse_file *in, gunichar *car)
{
  GError * error = NULL;
  GIOStatus status;

  do {

    status = g_io_channel_read_unichar(in->channel, car, &error);
    
    if(error != NULL) {
      g_warning("Can not read file: \n%s\n", error->message);
      g_error_free(error);
      break;
    }
    in->pos++;
   
    if(!g_unichar_isspace(*car)) {
      g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
      if(error != NULL) {
        g_warning("Can not seek file: \n%s\n", error->message);
        g_error_free(error);
      }
      in->pos--;
      break;
    }

  } while(status != G_IO_STATUS_EOF && status != G_IO_STATUS_ERROR);

  return status;
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

gchar *getToken(parse_file * in)
{
  gunichar car, car1;
  int pos = 0, escaped;
  GError * error = NULL;

  if(in->unget == 1) {
    in->unget = 0;
    return in->buffer;
  }

  /* skip whitespace and comments */

  while((g_io_channel_read_unichar(in->channel, &car, &error))
      != G_IO_STATUS_EOF) {

    if(error != NULL) {
      g_warning("Can not read file: \n%s\n", error->message);
      g_error_free(error);
    } else in->pos++;

    if(car == '/') {
      g_io_channel_read_unichar(in->channel, &car1, &error);
      if(error != NULL) {
        g_warning("Can not read file: \n%s\n", error->message);
        g_error_free(error);
      } else in->pos++;

      if(car1 == '*') skipComment(in);
      else if(car1 == '/') skipEOL(in);
      else {
        g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
        if(error != NULL) {
          g_warning("Can not seek file: \n%s\n", error->message);
          g_error_free(error);
        } else in->pos--;
        break;
      }
    }
    else if(car == '\n') in->lineno++;
    else if(!g_unichar_isspace(car)) break;
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
      while(g_io_channel_read_unichar(in->channel, &car, &error)
                  != G_IO_STATUS_EOF && pos < BUFFER_SIZE) {

        if(error != NULL) {
          g_warning("Can not read file: \n%s\n", error->message);
          g_error_free(error);
        } else in->pos++;

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
      if(g_unichar_isdigit(car)) {
        in->buffer[pos] = car;
        pos++;
        while(g_io_channel_read_unichar(in->channel, &car, &error)
                    != G_IO_STATUS_EOF && pos < BUFFER_SIZE) {

          if(error != NULL) {
            g_warning("Can not read file: \n%s\n", error->message);
            g_error_free(error);
          } else in->pos++;

          if(!isdigit(car)) {
            g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
            if(error != NULL) {
              g_warning("Can not seek file: \n%s\n", error->message);
              g_error_free(error);
            } else in->pos--;
            break;
          }
          in->buffer[pos] = car;
          pos++;
        }
	      if(car == EOF) {
          g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
          if(error != NULL) {
            g_warning("Can not seek file: \n%s\n", error->message);
            g_error_free(error);
          } else in->pos--;
        }
        if(pos == BUFFER_SIZE) in->error(in, "number token too large");
        in->type = NUMBER;
      }    
      else if(g_unichar_isalpha(car)) {
        in->buffer[0] = car;
        pos = 1;
        while(g_io_channel_read_unichar(in->channel, &car, &error)
                              != G_IO_STATUS_EOF && pos < BUFFER_SIZE) {

          if(error != NULL) {
            g_warning("Can not read file: \n%s\n", error->message);
            g_error_free(error);
          } else in->pos++;

          if(!(g_unichar_isalnum(car) || car == '_')) {
            g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
            if(error != NULL) {
              g_warning("Can not seek file: \n%s\n", error->message);
              g_error_free(error);
            } else in->pos--;
            break;
          }
          in->buffer[pos] = car;
          pos++;
        }
	      if(car == EOF) {
          g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
          if(error != NULL) {
            g_warning("Can not seek file: \n%s\n", error->message);
            g_error_free(error);
          } else in->pos--;
        }
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
  gunichar car;
  GError * error = NULL;

  while(g_io_channel_read_unichar(in->channel, &car, &error)
                                    != G_IO_STATUS_EOF) {

    if(error != NULL) {
      g_warning("Can not read file: \n%s\n", error->message);
      g_error_free(error);
    } else in->pos++;

    if(car == '\n') in->lineno++;
    else if(car == '*') {

      g_io_channel_read_unichar(in->channel, &car, &error);
      if(error != NULL) {
        g_warning("Can not read file: \n%s\n", error->message);
        g_error_free(error);
      } else in->pos++;

      if(car ==EOF) break;
      if(car == '/') return;

      g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
      if(error != NULL) {
        g_warning("Can not seek file: \n%s\n", error->message);
        g_error_free(error);
      } else in->pos--;

    }
  }
  if(car == EOF) in->error(in,"comment begining with '/*' has no ending '*/'");
}

void skipEOL(parse_file * in)
{
  gunichar car;
  GError * error = NULL;

  while(g_io_channel_read_unichar(in->channel, &car, &error)
                                          != G_IO_STATUS_EOF) {
    if(car == '\n') {
      g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
      if(error != NULL) {
        g_warning("Can not seek file: \n%s\n", error->message);
        g_error_free(error);
      } else in->pos--;
      break;
    }
  }
  if(car == EOF) {
    g_io_channel_seek_position(in->channel, -1, G_SEEK_CUR, &error);
    if(error != NULL) {
      g_warning("Can not seek file: \n%s\n", error->message);
      g_error_free(error);
    } else in->pos--;
  }
}

/*****************************************************************************
 *Function name
 *    checkNamedTypesImplemented : check if all named types have definition
 *    returns -1 on error, 0 if ok
 ****************************************************************************/

int checkNamedTypesImplemented(table * named_types)
{
  type_descriptor *t;
  int pos;
  char str[256];

  for(pos = 0 ; pos < named_types->values.position; pos++) {
    t = (type_descriptor *) named_types->values.array[pos];
    if(t->type == NONE){
      sprintf(str,"named type '%s' has no definition",
              (char*)named_types->keys.array[pos]);
      error_callback(NULL,str);
      return -1;
    }
  }
  return 0;
}


/*****************************************************************************
 *Function name
 *    generateChecksum  : generate checksum for the facility
 *Input Params
 *    facName           : name of facility
 *Output Params
 *    checksum          : checksum for the facility
 ****************************************************************************/

int generateChecksum(char* facName, guint32 * checksum, sequence * events)
{
  unsigned long crc ;
  int pos;
  event_t * ev;
  char str[256];

  crc = crc32(facName);
  for(pos = 0; pos < events->position; pos++){
    ev = (event_t *)(events->array[pos]);
    crc = partial_crc32(ev->name,crc);    
    if(!ev->type) continue; //event without type
    if(ev->type->type != STRUCT){
      sprintf(str,"event '%s' has a type other than STRUCT",ev->name);
      error_callback(NULL, str);
      return -1;
    }
    crc = getTypeChecksum(crc, ev->type);
  }
  *checksum = crc;
  return 0;
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
  type_fields * fld;

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
      str = g_strdup("string");
      flag = 1;
      break;
    case ENUM:
      str = g_strconcat("enum ", uintOutputTypes[type->size], NULL);
      flag = 1;
      break;
    case ARRAY:
      sprintf(buf,"%d",type->size);
      str = g_strconcat("array ",buf, NULL);
      flag = 1;
      break;
    case SEQUENCE:
      sprintf(buf,"%d",type->size);
      str = g_strconcat("sequence ",buf, NULL);
      flag = 1;
      break;
    case STRUCT:
      str = g_strdup("struct");
      flag = 1;
      break;
    case UNION:
      str = g_strdup("union");
      flag = 1;
      break;
    default:
      error_callback(NULL, "named type has no definition");
      break;
  }

  crc = partial_crc32(str,crc);
  if(flag) g_free(str);

  if(type->fmt) crc = partial_crc32(type->fmt,crc);
    
  if(type->type == ARRAY || type->type == SEQUENCE){
    crc = getTypeChecksum(crc,type->nested_type);
  }else if(type->type == STRUCT || type->type == UNION){
    for(pos =0; pos < type->fields.position; pos++){
      fld = (type_fields *) type->fields.array[pos];
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
  type_fields *f;

  if(tp->fmt != NULL) g_free(tp->fmt);
  if(tp->type == ENUM) {
    for(pos2 = 0; pos2 < tp->labels.position; pos2++) {
      g_free(tp->labels.array[pos2]);
    }
    sequence_dispose(&(tp->labels));
  }
  if(tp->type == STRUCT) {
    for(pos2 = 0; pos2 < tp->fields.position; pos2++) {
      f = (type_fields *) tp->fields.array[pos2];
      g_free(f->name);
      g_free(f->description);
      g_free(f);
    }
    sequence_dispose(&(tp->fields));
  }
}

void freeNamedType(table * t)
{
  int pos;
  type_descriptor * td;

  for(pos = 0 ; pos < t->keys.position; pos++) {
    g_free((char *)t->keys.array[pos]);
    td = (type_descriptor*)t->values.array[pos];
    freeType(td);
    g_free(td);
  }
}

void freeTypes(sequence *t) 
{
  int pos;
  type_descriptor *tp;

  for(pos = 0 ; pos < t->position; pos++) {
    tp = (type_descriptor *)t->array[pos];
    freeType(tp);
    g_free(tp);
  }
}

void freeEvents(sequence *t) 
{
  int pos;
  event_t *ev;

  for(pos = 0 ; pos < t->position; pos++) {
    ev = (event_t *) t->array[pos];
    g_free(ev->name);
    g_free(ev->description);
    g_free(ev);
  }

}


/* Extensible array */

void sequence_init(sequence *t) 
{
  t->size = 10;
  t->position = 0;
  t->array = g_new(void*, t->size);
}

void sequence_dispose(sequence *t) 
{
  t->size = 0;
  g_free(t->array);
  t->array = NULL;
}

void sequence_push(sequence *t, void *elem) 
{
  void **tmp;

  if(t->position >= t->size) {
    tmp = t->array;
    t->array = g_new(void*, 2*t->size);
    memcpy(t->array, tmp, t->size * sizeof(void *));
    t->size = t->size * 2;
    g_free(tmp);
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
    if(g_ascii_strcasecmp((char *)key,(char *)t->keys.array[pos]) == 0)
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


