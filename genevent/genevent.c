/*

genevent.c: Generate helper declarations and functions to trace events
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

/* This program reads the ".event" event definitions input files 
   specified as command line arguments and generates corresponding
   ".c" and ".h" files required to trace such events in the kernel.
 
   The program uses a very simple tokenizer, called from a hand written
   recursive descent parser to fill a data structure describing the events.
   The result is a sequence of events definitions which refer to type
   definitions.

   A table of named types is maintained to allow refering to types by name
   when the same type is used at several places. Finally a sequence of
   all types is maintained to facilitate the freeing of all type 
   information when the processing of an ".event" file is finished. */

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <linux/errno.h>  


#include "parser.h"
#include "genevent.h"

/* Named types may be referenced from anywhere */

facility * fac;

int main(int argc, char** argv)
{
  char *token;
  parse_file in;
  char buffer[BUFFER_SIZE];
  int i;

  if(argc < 2){
    printf("At least one event definition file is needed\n");
    exit(1);
  }

  in.buffer = buffer;
  in.error = error_callback;

  for(i = 1 ; i < argc ; i++) {
    in.lineno = 0;
    in.name = allocAndCopy(argv[i]);

    in.fp = fopen(in.name, "r");
    if(!in.fp ){
      in.error(&in,"cannot open facility input file");
    }

    while(1){
      token = getToken(&in);
      if(in.type == ENDFILE) break;

      if(strcmp(token, "<")) in.error(&in,"not a facility file");
      token = getName(&in);
    
      if(strcmp("facility",token) == 0) {
	fac = memAlloc(sizeof(facility));
	fac->name = NULL;
	fac->description = NULL;
	sequence_init(&(fac->events));
	table_init(&(fac->named_types));
	sequence_init(&(fac->unnamed_types));
	
	parseFacility(&in, fac);
	
	//check if any namedType is not defined
	checkNamedTypesImplemented(&fac->named_types);
      }else in.error(&in,"facility token was expected");

      generateFile(argv[i]);

      free(fac->name);
      free(fac->description);
      freeEvents(&fac->events);
      sequence_dispose(&fac->events);
      freeNamedType(&fac->named_types);
      table_dispose(&fac->named_types);
      freeTypes(&fac->unnamed_types);
      sequence_dispose(&fac->unnamed_types);      
      free(fac);      
    }

    free(in.name);
    fclose(in.fp);

  }
  return 0;
}


/*****************************************************************************
 *Function name
 *    generateFile : generate .c and .h file 
 *Input Params
 *    name         : name of event definition file
 ****************************************************************************/
void generateFile(char *name){
  char *cName, *hName, *tmp;
  FILE * cFp, *hFp;
  int nbEvent;
  unsigned long checksum=0;

  //remove .xml if it exists
  tmp = &name[strlen(name)-4];
  if(strcmp(tmp, ".xml") == 0){
    *tmp = '\0';
  }

  tmp = strrchr(name,'/');
  if(tmp){
    tmp++;
  }else{
    tmp = name;
  }
  
  cName = appendString(tmp,".c");
  hName = appendString(tmp,".h");
  cFp = fopen(cName,"w");
  if(!cFp){
    printf("Cannot open the file : %s\n",cName);
    exit(1);
  }

  hFp = fopen(hName,"w");
  if(!hFp){
     printf("Cannot open the file : %s\n",hName);
     exit(1);
  }
  
  free(cName);
  free(hName);

  generateChecksum(fac->name, &checksum, &(fac->events));

  /* generate .h file, event enumeration then structures and functions */
  generateEnumEvent(hFp, fac->name, &nbEvent);
  generateStructFunc(hFp, fac->name);

  /* generate .c file, calls to register the facility at init time */
  generateCfile(cFp,fac->name,nbEvent,checksum);

  fclose(hFp);
  fclose(cFp);
}


/*****************************************************************************
 *Function name
 *    generateEnumEvent : output event enum to .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 *Output Params
 *    nbEvent           : number of events in the facility
 ****************************************************************************/
void generateEnumEvent(FILE *fp, char *facName, int * nbEvent) {
  int pos = 0;

  //will be removed later
  fprintf(fp,"typedef unsigned int trace_facility_t;\n\n");
  fprintf(fp,"extern int trace_new(trace_facility_t fID, u8 eID, int length, char * buf);\n\n");


  fprintf(fp,"/****  facility handle  ****/\n\n");
  fprintf(fp,"extern trace_facility_t facility_%s;\n\n\n",facName);

  fprintf(fp,"/****  event type  ****/\n\n");
  fprintf(fp,"enum %s_event {\n",facName);

  for(pos = 0; pos < fac->events.position;pos++) {
    fprintf(fp,"  %s", ((event *)(fac->events.array[pos]))->name);
    if(pos != fac->events.position-1) fprintf(fp,",\n");
  }
  fprintf(fp,"\n};\n\n\n");

  //  fprintf(fp,"/****  number of events in the facility  ****/\n\n");
  //  fprintf(fp,"int nbEvents_%s = %d;\n\n\n",facName, fac->events.position);
  *nbEvent = fac->events.position;
}


/*****************************************************************************
 *Function name
 *    generateEnumDefinition: generate enum definition if it exists 
 *Input Params
 *    fp                    : file to be written to
 *    fHead                 : enum type
 ****************************************************************************/
void generateEnumDefinition(FILE * fp, type_descriptor * type){
  int pos;

  fprintf(fp,"enum {\n");
  for(pos = 0; pos < type->labels.position; pos++){
    fprintf(fp,"  %s", type->labels.array[pos]);
    if (pos != type->labels.position - 1) fprintf(fp,",\n");
  }
  fprintf(fp,"\n};\n\n\n");
}

/*****************************************************************************
 *Function name
 *    generateStrucTFunc: output structure and function to .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 ****************************************************************************/
void generateStructFunc(FILE * fp, char * facName){
  event * ev;
  field * fld;
  type_descriptor * td;
  int pos, pos1;
  int hasStrSeq, flag, structCount, seqCount,strCount, whichTypeFirst=0;

  for(pos = 0; pos < fac->events.position; pos++){
    ev = (event *) fac->events.array[pos];
    //yxx    if(ev->nested)continue;
    fprintf(fp,"/****  structure and trace function for event: %s  ****/\n\n",ev->name);
    if(ev->type == 0){ // event without type
      fprintf(fp,"static inline void trace_%s_%s(void){\n",facName,ev->name);
      fprintf(fp,"  trace_new(facility_%s, %s, 0, NULL);\n",facName,ev->name);
      fprintf(fp,"};\n\n\n");
      continue;
    }

    //if fields contain enum, print out enum definition
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld = (field *)ev->type->fields.array[pos1];
      if(fld->type->type == ENUM) generateEnumDefinition(fp, fld->type);      
    }
      
    //default: no string, array or sequence in the event
    hasStrSeq = 0;
    whichTypeFirst = 0;
    
    //structure for kernel
    flag = 0;
    structCount = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if( td->type != STRING && td->type != SEQUENCE && td->type != ARRAY){
	if(whichTypeFirst == 0) whichTypeFirst = 1; //struct first
	if(flag == 0){
	  flag = 1;	    
	  fprintf(fp,"struct %s_%s_%d{\n",ev->name,facName,++structCount);
	}
	fprintf(fp, "  %s %s; /* %s */\n",getTypeStr(td),fld->name,fld->description );
      }else{
	if(whichTypeFirst == 0) whichTypeFirst = 2; //string or sequence or array first
	hasStrSeq++;
	if(flag)
	  fprintf(fp,"} __attribute__ ((packed));\n\n");
	flag = 0;
      }       
    }
    if(flag)
      fprintf(fp,"} __attribute__ ((packed));\n\n");

    //trace function : function name and parameters
    seqCount = 0;
    strCount = 0;
    fprintf(fp,"static inline void trace_%s_%s(",facName,ev->name);
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type == ARRAY ){
	fprintf(fp,"%s * %s",getTypeStr(td), fld->name);
      }else if(td->type == STRING){
 	fprintf(fp,"short int strLength_%d, %s * %s",++strCount, getTypeStr(td), fld->name);
     }else if(td->type == SEQUENCE){
	  fprintf(fp,"%s seqLength_%d, %s * %s",uintOutputTypes[td->size], ++seqCount,getTypeStr(td), fld->name);
      }else  fprintf(fp,"%s %s",getTypeStr(td), fld->name);     
      if(pos1 != ev->type->fields.position -1)fprintf(fp,", ");
    }
    fprintf(fp,"){\n");

    //length of buffer : length of all structures
    fprintf(fp,"  int bufLength = ");
    for(pos1=0;pos1<structCount;pos1++){
      fprintf(fp,"sizeof(struct %s_%s_%d)",ev->name, facName,pos1+1);
      if(pos1 != structCount-1) fprintf(fp," + ");
    }

    //length of buffer : length of all arrays, sequences and strings
    seqCount = 0;
    strCount = 0;
    flag = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type == SEQUENCE || td->type==STRING ||td->type==ARRAY){
	if(structCount || flag > 0) fprintf(fp," + ");	  
	if(td->type == SEQUENCE) fprintf(fp,"sizeof(%s) + sizeof(%s) * seqLength_%d",uintOutputTypes[td->size], getTypeStr(td), ++seqCount);
	else if(td->type==STRING) fprintf(fp,"strLength_%d + 1", ++strCount);
	else if(td->type==ARRAY) fprintf(fp,"sizeof(%s) * %d", getTypeStr(td),td->size);
	if(structCount == 0) flag = 1;
      }
    }
    fprintf(fp,";\n");

    //allocate buffer
    fprintf(fp,"  char buff[bufLength];\n");

    //declare a char pointer if needed
    if(structCount + hasStrSeq > 1) fprintf(fp,"  char * ptr = buff;\n");
    
    //allocate memory for new struct and initialize it
    if(whichTypeFirst == 1){ //struct first
      for(pos1=0;pos1<structCount;pos1++){	
	if(pos1==0) fprintf(fp,"  struct %s_%s_1 * __1 = (struct %s_%s_1 *)buff;\n",ev->name, facName,ev->name, facName);
	else fprintf(fp,"  struct %s_%s_%d  __%d;\n",ev->name, facName,pos1+1,pos1+1);	
      }      
    }else if(whichTypeFirst == 2){
      for(pos1=0;pos1<structCount;pos1++)
	fprintf(fp,"  struct %s_%s_%d  __%d;\n",ev->name, facName,pos1+1,pos1+1);
    }
    fprintf(fp,"\n");

    if(structCount) fprintf(fp,"  //initialize structs\n");
    flag = 0;
    structCount = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type != ARRAY && td->type != SEQUENCE && td->type != STRING){
	if(flag == 0){
	  flag = 1;	
	  structCount++;
	  if(structCount > 1) fprintf(fp,"\n");
	}
	if(structCount == 1 && whichTypeFirst == 1) fprintf(fp, "  __1->%s =  %s;\n",fld->name,fld->name );
	else fprintf(fp, "  __%d.%s =  %s;\n",structCount ,fld->name,fld->name);	
      }else flag = 0;
    }
    if(structCount) fprintf(fp,"\n");

    //set ptr to the end of first struct if needed;
    if(whichTypeFirst == 1 && structCount + hasStrSeq > 1){
      fprintf(fp,"\n  //set ptr to the end of the first struct\n");
      fprintf(fp,"  ptr +=  sizeof(struct %s_%s_1);\n\n",ev->name, facName); 
    }

    //copy struct, sequence and string to buffer
    seqCount = 0;
    strCount = 0;
    flag = 0;
    structCount = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type != STRING && td->type != SEQUENCE && td->type != ARRAY){
	if(flag == 0) structCount++;	
	flag++;	
	if((structCount > 1 || whichTypeFirst == 2) && flag == 1){
	  fprintf(fp,"  //copy struct to buffer\n");
	  fprintf(fp,"  memcpy(ptr, &__%d, sizeof(struct %s_%s_%d));\n",structCount, ev->name, facName,structCount);
	  fprintf(fp,"  ptr +=  sizeof(struct %s_%s_%d);\n\n",ev->name, facName,structCount); 
	}
      }else if(td->type == SEQUENCE){
	flag = 0;
	fprintf(fp,"  //copy sequence length and sequence to buffer\n");
	fprintf(fp,"  *ptr = seqLength_%d;\n",++seqCount);
	fprintf(fp,"  ptr += sizeof(%s);\n",uintOutputTypes[td->size]);
	fprintf(fp,"  memcpy(ptr, %s, sizeof(%s) * seqLength_%d);\n",fld->name, getTypeStr(td), seqCount);
	fprintf(fp,"  ptr += sizeof(%s) * seqLength_%d;\n\n",getTypeStr(td), seqCount );
      }else if(td->type==STRING){
	flag = 0;
	fprintf(fp,"  //copy string to buffer\n");
	fprintf(fp,"  if(strLength_%d > 0){\n",++strCount);
	fprintf(fp,"    memcpy(ptr, %s, strLength_%d + 1);\n", fld->name, strCount);
	fprintf(fp,"    ptr += strLength_%d + 1;\n",strCount);	
	fprintf(fp,"  }else{\n");
	fprintf(fp,"    *ptr = '\\0';\n");
	fprintf(fp,"    ptr += 1;\n");
	fprintf(fp,"  }\n\n");
      }else if(td->type==ARRAY){
	flag = 0;
	fprintf(fp,"  //copy array to buffer\n");
	fprintf(fp,"  memcpy(ptr, %s, sizeof(%s) * %d);\n", fld->name, getTypeStr(td), td->size);
	fprintf(fp,"  ptr += sizeof(%s) * %d;\n\n",getTypeStr(td), td->size);
      }      
    }    
    if(structCount + seqCount > 1) fprintf(fp,"\n");

    //call trace function
    fprintf(fp,"\n  //call trace function\n");
    fprintf(fp,"  trace_new(facility_%s, %s, bufLength, buff);\n",facName,ev->name);
    fprintf(fp,"};\n\n\n");
  }

}

/*****************************************************************************
 *Function name
 *    getTypeStr        : generate type string 
 *Input Params
 *    td                : a type descriptor
 *Return Values
 *    char *            : type string
 ****************************************************************************/
char * getTypeStr(type_descriptor * td){
  type_descriptor * t ;

  switch(td->type){
    case INT:
      return intOutputTypes[td->size];
    case UINT:
      return uintOutputTypes[td->size];
    case FLOAT:
      return floatOutputTypes[td->size];
    case STRING:
      return "char";
    case ENUM:
      return uintOutputTypes[td->size];
    case ARRAY:
    case SEQUENCE:
      t = td->nested_type;
      switch(t->type){
        case INT:
	  return intOutputTypes[t->size];
        case UINT:
	  return uintOutputTypes[t->size];
        case FLOAT:
	  return floatOutputTypes[t->size];
        case STRING:
	  return "char";
        case ENUM:
	  return uintOutputTypes[t->size];
        default :
	  error_callback(NULL,"Nested struct is not supportted");
	  break;	
      }
      break;
    case STRUCT: //for now we do not support nested struct
      error_callback(NULL,"Nested struct is not supportted");
      break;
    default:
      error_callback(NULL,"No type information");
      break;
  }
  return NULL;
}

/*****************************************************************************
 *Function name
 *    generateCfile     : generate a .c file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 *    nbEvent           : number of events in the facility
 *    checksum          : checksum for the facility
 ****************************************************************************/
void generateCfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum){
  //will be removed later
  fprintf(fp,"typedef unsigned int trace_facility_t;\n\n");

  fprintf(fp,"static unsigned long checksum = %lu;\n\n",checksum);
  fprintf(fp,"/* facility handle */\n");
  fprintf(fp,"trace_facility_t facility_%s;\n\n",facName);

  fprintf(fp,"static void __init facility_%s_init(){\n",facName);  
  fprintf(fp,"  facility_%s =  trace_register_facility_by_checksum(\"%s\", checksum,%d);\n",facName,facName,nbEvent);
  fprintf(fp,"}\n\n");

  fprintf(fp,"static void __exit facility_%s_exit(){\n",facName);  
  fprintf(fp,"}\n\n");

  fprintf(fp,"module_init(facility_%s_init);\n",facName);
  fprintf(fp,"module_exit(facility_%s_exit);\n",facName);  
}


