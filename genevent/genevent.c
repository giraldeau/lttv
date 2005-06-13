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
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <linux/errno.h>  
#include <assert.h>

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
      }
      else in.error(&in,"facility token was expected");

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
  char *loadName, *hName, *hIdName, *cName, *tmp, *tmp2;
  FILE * lFp, *hFp, *iFp, *cFp;
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

  loadName = appendString("ltt-facility-loader-", tmp);
  tmp2 = appendString(loadName,".h");
  free(loadName);
  loadName = tmp2;
  hName = appendString("ltt-facility-", tmp);
  tmp2 = appendString(hName,".h");
  free(hName);
  hName = tmp2;
  hIdName = appendString("ltt-facility-id-", tmp);
  tmp2 = appendString(hIdName,".h");
  free(hIdName);
  hIdName = tmp2;
  cName = appendString("ltt-facility-loader-", tmp);
  tmp2 = appendString(cName,".c");
  free(cName);
  cName = tmp2;
  lFp = fopen(loadName,"w");
  if(!lFp){
    printf("Cannot open the file : %s\n",loadName);
    exit(1);
  }

  hFp = fopen(hName,"w");
  if(!hFp){
     printf("Cannot open the file : %s\n",hName);
     exit(1);
  }

  iFp = fopen(hIdName,"w");
  if(!iFp){
     printf("Cannot open the file : %s\n",hIdName);
     exit(1);
  }
  
  cFp = fopen(cName,"w");
  if(!cFp){
     printf("Cannot open the file : %s\n",cName);
     exit(1);
  }
  
  free(loadName);
  free(hName);
  free(hIdName);
  free(cName);

  generateChecksum(fac->name, &checksum, &(fac->events));

  /* generate .h file, event enumeration then structures and functions */
  fprintf(iFp, "#ifndef _LTT_FACILITY_ID_%s_H_\n",fac->capname);
  fprintf(iFp, "#define _LTT_FACILITY_ID_%s_H_\n\n",fac->capname);
  generateEnumEvent(iFp, fac->name, &nbEvent, checksum);
  fprintf(iFp, "#endif //_LTT_FACILITY_ID_%s_H_\n",fac->capname);


  fprintf(hFp, "#ifndef _LTT_FACILITY_%s_H_\n",fac->capname);
  fprintf(hFp, "#define _LTT_FACILITY_%s_H_\n\n",fac->capname);
  generateTypeDefs(hFp, fac->name);
  generateStructFunc(hFp, fac->name,checksum);
  fprintf(hFp, "#endif //_LTT_FACILITY_%s_H_\n",fac->capname);

  /* generate .h file, calls to register the facility at init time */
  generateLoaderfile(lFp,fac->name,nbEvent,checksum,fac->capname);

  // create ltt-facility-loader-facname.c
  generateCfile(cFp, tmp);

  fclose(hFp);
  fclose(iFp);
  fclose(lFp);
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
void generateEnumEvent(FILE *fp, char *facName, int * nbEvent, unsigned long checksum) {
  int pos = 0;

  fprintf(fp,"#include <linux/ltt-facilities.h>\n\n");

  fprintf(fp,"/****  facility handle  ****/\n\n");
  fprintf(fp,"extern ltt_facility_t ltt_facility_%s_%X;\n",facName, checksum);
  fprintf(fp,"extern ltt_facility_t ltt_facility_%s;\n\n\n",facName, checksum);

  fprintf(fp,"/****  event type  ****/\n\n");
  fprintf(fp,"enum %s_event {\n",facName);

  for(pos = 0; pos < fac->events.position;pos++) {
    fprintf(fp,"\tevent_%s", ((event *)(fac->events.array[pos]))->name);
    if(pos != fac->events.position-1) fprintf(fp,",\n");
  }
  fprintf(fp,"\n};\n\n\n");

  //  fprintf(fp,"/****  number of events in the facility  ****/\n\n");
  //  fprintf(fp,"int nbEvents_%s = %d;\n\n\n",facName, fac->events.position);
  *nbEvent = fac->events.position;
}


/*****************************************************************************
 *Function name
 *    printStruct       : Generic struct printing function
 *Input Params
 *    fp                : file to be written to
 *    len               : number of fields
 *    array             : array of field info
 *    name              : basic struct name
 *    facName           : name of facility
 *    whichTypeFirst    : struct or array/sequence first
 *    hasStrSeq         : string or sequence present?
 *    structCount       : struct postfix
 ****************************************************************************/

static void
printStruct(FILE * fp, int len, void ** array, char * name, char * facName,
       int * whichTypeFirst, int * hasStrSeq, int * structCount)
{
  int flag = 0;
  int pos;
  field * fld;
  type_descriptor * td;

  for (pos = 0; pos < len; pos++) {
    fld  = (field *)array[pos];
    td = fld->type;
    if( td->type == STRING || td->type == SEQUENCE ||
        td->type == ARRAY) {
        (*hasStrSeq)++;
		}
//      if (*whichTypeFirst == 0) {
//        *whichTypeFirst = 1; //struct first
//      }
      if (flag == 0) {
        flag = 1;

        fprintf(fp,"struct %s_%s",name, facName);
        if (structCount) {
          fprintf(fp, "_%d {\n",++*structCount);
        } else {
          fprintf(fp, " {\n");
        }
      }
      fprintf(fp, "\t%s %s; /* %s */\n", 
          getTypeStr(td),fld->name,fld->description );
#if 0
    } else {
        if (*whichTypeFirst == 0) {
        //string or sequence or array first
          *whichTypeFirst = 2;
        }
        (*hasStrSeq)++;
        if(flag) {
          fprintf(fp,"} __attribute__ ((packed));\n\n");
        }
        flag = 0;
    }
#endif //0
  }

  if(flag) {
    fprintf(fp,"} __attribute__ ((packed));\n\n");
  }
}


/*****************************************************************************
 *Function name
 *    generateHfile     : Create the typedefs
 *Input Params
 *    fp                : file to be written to
 ****************************************************************************/
void
generateTypeDefs(FILE * fp, char *facName)
{
  int pos, tmp = 1;

  fprintf(fp,"#include <linux/types.h>\n");
  fprintf(fp,"#include <linux/spinlock.h>\n");
  fprintf(fp,"#include <linux/ltt/ltt-facility-id-%s.h>\n\n", facName);
  fprintf(fp,"#include <linux/ltt-core.h>\n");

  fprintf(fp, "/****  Basic Type Definitions  ****/\n\n");

  for (pos = 0; pos < fac->named_types.values.position; pos++) {
    type_descriptor * type =
      (type_descriptor*)fac->named_types.values.array[pos];
    printStruct(fp, type->fields.position, type->fields.array,
                "", type->type_name, &tmp, &tmp, NULL);
    fprintf(fp, "typedef struct _%s %s;\n\n",
            type->type_name, type->type_name);
  }
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

  if(type->already_printed) return;
  
  fprintf(fp,"enum {\n");
  for(pos = 0; pos < type->labels.position; pos++){
    fprintf(fp,"\tLTT_ENUM_%s", type->labels.array[pos]);
    if (pos != type->labels.position - 1) fprintf(fp,",");
    if(type->labels_description.array[pos] != NULL)
      fprintf(fp,"\t/* %s */\n",type->labels_description.array[pos]);
    else
      fprintf(fp,"\n");
  }
  fprintf(fp,"};\n\n\n");

  type->already_printed = 1;
}

/*****************************************************************************
 *Function name
 *    generateStrucTFunc: output structure and function to .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 ****************************************************************************/
void generateStructFunc(FILE * fp, char * facName, unsigned long checksum){
  event * ev;
  field * fld;
  type_descriptor * td;
  int pos, pos1;
  int hasStrSeq, flag, structCount, seqCount,strCount, whichTypeFirst=0;

  for(pos = 0; pos < fac->events.position; pos++){
    ev = (event *) fac->events.array[pos];
    //yxx    if(ev->nested)continue;
    fprintf(fp,"/****  structure and trace function for event: %s  ****/\n\n",
        ev->name);
    //if(ev->type == 0){ // event without type
    //  fprintf(fp,"static inline void trace_%s_%s(void){\n",facName,ev->name);
    //  fprintf(fp,"\tltt_log_event(ltt_facility_%s_%X, event_%s, 0, NULL);\n",
    //      facName,checksum,ev->name);
    //  fprintf(fp,"};\n\n\n");
    //  continue;
    //}

    //if fields contain enum, print out enum definition
    //MD : fixed in generateEnumDefinition to do not print the same enum
    //twice.
    if(ev->type != 0)
      for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
        fld = (field *)ev->type->fields.array[pos1];
        if(fld->type->type == ENUM) generateEnumDefinition(fp, fld->type);      
      }
      
    //default: no string, array or sequence in the event
    hasStrSeq = 0;
    whichTypeFirst = 0;
    structCount = 0;

    //structure for kernel
    if(ev->type != 0)
      printStruct(fp, ev->type->fields.position, ev->type->fields.array,
        ev->name, facName, &whichTypeFirst, &hasStrSeq, &structCount);

    //trace function : function name and parameters
    seqCount = 0;
    strCount = 0;
    fprintf(fp,"static inline void trace_%s_%s(",facName,ev->name);
    if(ev->type == 0)
      fprintf(fp, "void");
    else
      for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
        fld  = (field *)ev->type->fields.array[pos1];
        td = fld->type;
        if(td->type == ARRAY ){
          fprintf(fp,"%s * %s",getTypeStr(td), fld->name);
        }else if(td->type == STRING){
          fprintf(fp,"short int strlength_%d, %s * %s",
              ++strCount, getTypeStr(td), fld->name);
       }else if(td->type == SEQUENCE){
          fprintf(fp,"%s seqlength_%d, %s * %s",
              uintOutputTypes[td->size], ++seqCount,getTypeStr(td), fld->name);
       }else fprintf(fp,"%s %s",getTypeStr(td), fld->name);     
       if(pos1 != ev->type->fields.position - 1) fprintf(fp,", ");
      }
    fprintf(fp,")\n{\n");

    //length of buffer : length of all structures
		fprintf(fp,"\tint length = ");
    if(ev->type == 0) fprintf(fp, "0");

    for(pos1=0;pos1<structCount;pos1++){
      fprintf(fp,"sizeof(struct %s_%s_%d)",ev->name, facName,pos1+1);
      if(pos1 != structCount-1) fprintf(fp," + ");
    }

    //length of buffer : length of all arrays, sequences and strings
    seqCount = 0;
    strCount = 0;
    flag = 0;
		if(ev->type != 0)
			for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
				fld  = (field *)ev->type->fields.array[pos1];
				td = fld->type;
				if(td->type == SEQUENCE || td->type==STRING || td->type==ARRAY){
					if(structCount || flag > 0) fprintf(fp," + ");    
					if(td->type == SEQUENCE) 
						fprintf(fp,"sizeof(%s) + sizeof(%s) * seqlength_%d",
								uintOutputTypes[td->size], getTypeStr(td), ++seqCount);
					else if(td->type==STRING) fprintf(fp,"strlength_%d + 1", ++strCount);
					else if(td->type==ARRAY) 
						fprintf(fp,"sizeof(%s) * %d", getTypeStr(td),td->size);
					if(structCount == 0) flag = 1;
				}
			}
    fprintf(fp,";\n");

    //allocate buffer
    // MD no more need. fprintf(fp,"\tchar buff[buflength];\n");
    // write directly to the channel
    fprintf(fp, "\tunsigned int index;\n");
    fprintf(fp, "\tstruct ltt_channel_struct *channel;\n");
    fprintf(fp, "\tstruct ltt_trace_struct *trace;\n");
    fprintf(fp, "\tunsigned long _flags;\n");
		if(ev->type != 0)
      fprintf(fp, "\tstruct %s_%s_1* __1;\n\n", ev->name, facName);

    fprintf(fp, "\tread_lock(&ltt_traces.traces_rwlock);\n\n");
    fprintf(fp,
        "\tif(ltt_traces.num_active_traces == 0) goto unlock_traces;\n\n");

    fprintf(fp, 
        "\tindex = ltt_get_index_from_facility(ltt_facility_%s_%X,\n"\
            "\t\t\t\tevent_%s);\n",
        facName, checksum, ev->name);
    fprintf(fp,"\n");

    fprintf(fp, "\t/* Disable interrupts. */\n");
    fprintf(fp, "\tlocal_irq_save(_flags);\n\n");

    /* For each trace */
    fprintf(fp, "\tlist_for_each_entry(trace, &ltt_traces.head, list) {\n");
    fprintf(fp, "\t\tif(!trace->active) continue;\n\n");
    
    fprintf(fp, "\t\tunsigned int header_length = "
                "ltt_get_event_header_size(trace);\n");
    fprintf(fp, "\t\tunsigned int event_length = header_length + length;\n");
   
    /* Reserve the channel */
    fprintf(fp, "\t\tchannel = ltt_get_channel_from_index(trace, index);\n");
    fprintf(fp,
        "\t\tvoid *buff = relay_reserve(channel->rchan, event_length);\n");
    fprintf(fp, "\t\tif(buff == NULL) {\n");
    fprintf(fp, "\t\t\t/* Buffer is full*/\n");
    fprintf(fp, "\t\t\t/* for debug BUG(); */\n"); // DEBUG!
    fprintf(fp, "\t\t\tchannel->events_lost[smp_processor_id()]++;\n");
    fprintf(fp, "\t\t\tgoto commit_work;\n");
    fprintf(fp, "\t\t}\n");
		
		/* DEBUG */
		fprintf(fp, "/* for debug printk(\"f%%lu e\%%u \", ltt_facility_%s_%X, event_%s); */",
				facName, checksum, ev->name);

    /* Write the header */
    fprintf(fp, "\n");
    fprintf(fp, "\t\tltt_write_event_header(trace, channel, buff, \n"
                "\t\t\t\tltt_facility_%s_%X, event_%s, length);\n",
                facName, checksum, ev->name);
    fprintf(fp, "\n");
    
    //declare a char pointer if needed : starts at the end of the structs.
    if(structCount + hasStrSeq > 1) {
      fprintf(fp,"\t\tchar * ptr = (char*)buff + header_length");
      for(pos1=0;pos1<structCount;pos1++){
        fprintf(fp," + sizeof(struct %s_%s_%d)",ev->name, facName,pos1+1);
      }
      if(structCount + hasStrSeq > 1) fprintf(fp,";\n");
    }

    // Declare an alias pointer of the struct type to the beginning
    // of the reserved area, just after the event header.
		if(ev->type != 0)
      fprintf(fp, "\t\t__1 = (struct %s_%s_1 *)(buff + header_length);\n",
          ev->name, facName);
    //allocate memory for new struct and initialize it
    //if(whichTypeFirst == 1){ //struct first
      //for(pos1=0;pos1<structCount;pos1++){  
      //  if(pos1==0) fprintf(fp,
      //      "\tstruct %s_%s_1 * __1 = (struct %s_%s_1 *)buff;\n",
      //      ev->name, facName,ev->name, facName);
  //MD disabled      else fprintf(fp,
  //          "\tstruct %s_%s_%d  __%d;\n",
  //          ev->name, facName,pos1+1,pos1+1);
      //}      
    //}else if(whichTypeFirst == 2){
     // for(pos1=0;pos1<structCount;pos1++)
     //   fprintf(fp,"\tstruct %s_%s_%d  __%d;\n",
     //       ev->name, facName,pos1+1,pos1+1);
    //}
    fprintf(fp,"\n");

    if(structCount) fprintf(fp,"\t\t//initialize structs\n");
    //flag = 0;
    //structCount = 0;
		if(ev->type != 0)
			for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
				fld  = (field *)ev->type->fields.array[pos1];
				td = fld->type;
				if(td->type != ARRAY && td->type != SEQUENCE && td->type != STRING){
					//if(flag == 0){
					//  flag = 1;  
					//  structCount++;
					//  if(structCount > 1) fprintf(fp,"\n");
					//}
					fprintf(fp, "\t\t__1->%s = %s;\n", fld->name, fld->name );

				//if(structCount == 1 && whichTypeFirst == 1)
				//  fprintf(fp, "\t__1->%s =  %s;\n",fld->name,fld->name );
				//else 
				//  fprintf(fp, "\t__%d.%s =  %s;\n",structCount ,fld->name,fld->name);
				}
				//else flag = 0;
			}
    if(structCount) fprintf(fp,"\n");
    //set ptr to the end of first struct if needed;
    if(structCount + hasStrSeq > 1){
      fprintf(fp,"\n\t\t//set ptr to the end of the first struct\n");
      fprintf(fp,"\t\tptr +=  sizeof(struct %s_%s_1);\n\n",ev->name, facName);
    }

    //copy struct, sequence and string to buffer
    seqCount = 0;
    strCount = 0;
    flag = 0;
    structCount = 0;
		if(ev->type != 0)
			for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
				fld  = (field *)ev->type->fields.array[pos1];
				td = fld->type;
	//      if(td->type != STRING && td->type != SEQUENCE && td->type != ARRAY){
	//        if(flag == 0) structCount++;  
	//        flag++;  
	//        if((structCount > 1 || whichTypeFirst == 2) && flag == 1){
	//          assert(0); // MD : disabled !
	//          fprintf(fp,"\t//copy struct to buffer\n");
	//          fprintf(fp,"\tmemcpy(ptr, &__%d, sizeof(struct %s_%s_%d));\n",
	//              structCount, ev->name, facName,structCount);
	//          fprintf(fp,"\tptr +=  sizeof(struct %s_%s_%d);\n\n",
	//              ev->name, facName,structCount);
	//        }
	 //     }
				//else if(td->type == SEQUENCE){
				if(td->type == SEQUENCE){
					flag = 0;
					fprintf(fp,"\t\t//copy sequence length and sequence to buffer\n");
					fprintf(fp,"\t\t*ptr = seqlength_%d;\n",++seqCount);
					fprintf(fp,"\t\tptr += sizeof(%s);\n",uintOutputTypes[td->size]);
					fprintf(fp,"\t\tmemcpy(ptr, %s, sizeof(%s) * seqlength_%d);\n",
						fld->name, getTypeStr(td), seqCount);
					 fprintf(fp,"\t\tptr += sizeof(%s) * seqlength_%d;\n\n",
						getTypeStr(td), seqCount);
				}
				else if(td->type==STRING){
					flag = 0;
					fprintf(fp,"\t\t//copy string to buffer\n");
					fprintf(fp,"\t\tif(strlength_%d > 0){\n",++strCount);
					fprintf(fp,"\t\t\tmemcpy(ptr, %s, strlength_%d + 1);\n",
							fld->name, strCount);
					fprintf(fp,"\t\t\tptr += strlength_%d + 1;\n",strCount);
					fprintf(fp,"\t\t}else{\n");
					fprintf(fp,"\t\t\t*ptr = '\\0';\n");
					fprintf(fp,"\t\t\tptr += 1;\n");
					fprintf(fp,"\t\t}\n\n");
				}else if(td->type==ARRAY){
					flag = 0;
					fprintf(fp,"\t//copy array to buffer\n");
					fprintf(fp,"\tmemcpy(ptr, %s, sizeof(%s) * %d);\n",
							fld->name, getTypeStr(td), td->size);
					fprintf(fp,"\tptr += sizeof(%s) * %d;\n\n", getTypeStr(td), td->size);
				}      
			}
    if(structCount + seqCount > 1) fprintf(fp,"\n");

    fprintf(fp,"\n");
    fprintf(fp,"commit_work:\n");
    fprintf(fp,"\n");
    fprintf(fp, "\t\t/* Commit the work */\n");
    fprintf(fp, "\t\trelay_commit(channel->rchan->buf[smp_processor_id()],\n"
        "\t\t\t\tbuff, event_length);\n");

   /* End of traces iteration */
    fprintf(fp, "\t}\n\n");

    fprintf(fp, "\t/* Re-enable interrupts */\n");
    fprintf(fp, "\tlocal_irq_restore(_flags);\n");
    fprintf(fp, "\tpreempt_check_resched();\n");
    
    fprintf(fp, "\n");
    fprintf(fp, "unlock_traces:\n");
    fprintf(fp, "\tread_unlock(&ltt_traces.traces_rwlock);\n");
    //call trace function
    //fprintf(fp,"\n\t//call trace function\n");
    //fprintf(fp,"\tltt_log_event(ltt_facility_%s_%X, %s, bufLength, buff);\n",facName,checksum,ev->name);
    fprintf(fp,"}\n\n\n");
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
    case POINTER:
      return "void *";
    case LONG:
      return "long";
    case ULONG:
      return "unsigned long";
    case SIZE_T:
      return "size_t";
    case SSIZE_T:
      return "ssize_t";
    case OFF_T:
      return "off_t";
    case FLOAT:
      return floatOutputTypes[td->size];
    case STRING:
      return "const char";
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
        case POINTER:
          return "void *";
        case LONG:
          return "long";
        case ULONG:
          return "unsigned long";
        case SIZE_T:
          return "size_t";
        case SSIZE_T:
          return "ssize_t";
        case OFF_T:
          return "off_t";
        case FLOAT:
          return floatOutputTypes[t->size];
        case STRING:
          return "const char";
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
 *    generateLoaderfile: generate a facility loaded .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 *    nbEvent           : number of events in the facility
 *    checksum          : checksum for the facility
 ****************************************************************************/
void generateLoaderfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum, char *capname){
  fprintf(fp, "#ifndef _LTT_FACILITY_LOADER_%s_H_\n",capname);
  fprintf(fp, "#define _LTT_FACILITY_LOADER_%s_H_\n\n",capname);
  fprintf(fp,"#include <linux/ltt-facilities.h>\n", facName, checksum);
  fprintf(fp,"ltt_facility_t\tltt_facility_%s;\n", facName, checksum);
  fprintf(fp,"ltt_facility_t\tltt_facility_%s_%X;\n\n", facName, checksum);

  fprintf(fp,"#define LTT_FACILITY_SYMBOL\t\t\t\t\t\t\tltt_facility_%s\n",
      facName);
  fprintf(fp,"#define LTT_FACILITY_CHECKSUM_SYMBOL\t\tltt_facility_%s_%X\n",
      facName, checksum);
  fprintf(fp,"#define LTT_FACILITY_CHECKSUM\t\t\t\t\t\t0x%X\n", checksum);
  fprintf(fp,"#define LTT_FACILITY_NAME\t\t\t\t\t\t\t\t\"%s\"\n", facName);
  fprintf(fp,"#define LTT_FACILITY_NUM_EVENTS\t\t\t\t\t%d\n\n", nbEvent);
  fprintf(fp, "#endif //_LTT_FACILITY_LOADER_%s_H_\n",capname);
}

void generateCfile(FILE * fp, char * filefacname){

  fprintf(fp, "/*\n");
  fprintf(fp, " * ltt-facility-loader-%s.c\n", filefacname);
  fprintf(fp, " *\n");
  fprintf(fp, " * (C) Copyright  2005 - \n");
  fprintf(fp, " *          Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)\n");
  fprintf(fp, " *\n");
  fprintf(fp, " * Contains the LTT facility loader.\n");
  fprintf(fp, " *\n");
  fprintf(fp, " */\n");
  fprintf(fp, "\n");
  fprintf(fp, "\n");
  fprintf(fp, "#include <linux/ltt-facilities.h>\n");
  fprintf(fp, "#include <linux/module.h>\n");
  fprintf(fp, "#include <linux/init.h>\n");
  fprintf(fp, "#include <linux/config.h>\n");
  fprintf(fp, "#include \"ltt-facility-loader-%s.h\"\n", filefacname);
  fprintf(fp, "\n");
  fprintf(fp, "\n");
  fprintf(fp, "#ifdef CONFIG_LTT\n");
  fprintf(fp, "\n");
  fprintf(fp, "EXPORT_SYMBOL(LTT_FACILITY_SYMBOL);\n");
  fprintf(fp, "EXPORT_SYMBOL(LTT_FACILITY_CHECKSUM_SYMBOL);\n");
  fprintf(fp, "\n");
  fprintf(fp, "static const char ltt_facility_name[] = LTT_FACILITY_NAME;\n");
  fprintf(fp, "\n");
  fprintf(fp, "#define SYMBOL_STRING(sym) #sym\n");
  fprintf(fp, "\n");
  fprintf(fp, "static struct ltt_facility facility = {\n");
  fprintf(fp, "\t.name = ltt_facility_name,\n");
  fprintf(fp, "\t.num_events = LTT_FACILITY_NUM_EVENTS,\n");
  fprintf(fp, "\t.checksum = LTT_FACILITY_CHECKSUM,\n");
  fprintf(fp, "\t.symbol = SYMBOL_STRING(LTT_FACILITY_SYMBOL)\n");
  fprintf(fp, "};\n");
  fprintf(fp, "\n");
  fprintf(fp, "#ifndef MODULE\n");
  fprintf(fp, "\n");
  fprintf(fp, "/* Built-in facility. */\n");
  fprintf(fp, "\n");
  fprintf(fp, "static int __init facility_init(void)\n");
  fprintf(fp, "{\n");
  fprintf(fp, "\tprintk(KERN_INFO \"LTT : ltt-facility-%s init in kernel\\n\");\n", filefacname);
  fprintf(fp, "\n");
  fprintf(fp, "\tLTT_FACILITY_SYMBOL = ltt_facility_builtin_register(&facility);\n");
  fprintf(fp, "\tLTT_FACILITY_CHECKSUM_SYMBOL = LTT_FACILITY_SYMBOL;\n");
  fprintf(fp, "\t\n");
  fprintf(fp, "\treturn LTT_FACILITY_SYMBOL;\n");
  fprintf(fp, "}\n");
  fprintf(fp, "__initcall(facility_init);\n");
  fprintf(fp, "\n");
  fprintf(fp, "\n");
  fprintf(fp, "\n");
  fprintf(fp, "#else \n");
  fprintf(fp, "\n");
  fprintf(fp, "/* Dynamic facility. */\n");
  fprintf(fp, "\n");
  fprintf(fp, "static int __init facility_init(void)\n");
  fprintf(fp, "{\n");
  fprintf(fp, "\tprintk(KERN_INFO \"LTT : ltt-facility-%s init dynamic\\n\");\n", filefacname);
  fprintf(fp, "\n");
  fprintf(fp, "\tLTT_FACILITY_SYMBOL = ltt_facility_dynamic_register(&facility);\n");
  fprintf(fp, "\tLTT_FACILITY_SYMBOL_CHECKSUM = LTT_FACILITY_SYMBOL;\n");
  fprintf(fp, "\n");
  fprintf(fp, "\treturn LTT_FACILITY_SYMBOL;\n");
  fprintf(fp, "}\n");
  fprintf(fp, "\n");
  fprintf(fp, "static void __exit facility_exit(void)\n");
  fprintf(fp, "{\n");
  fprintf(fp, "\tint err;\n");
  fprintf(fp, "\n");
  fprintf(fp, "\terr = ltt_facility_dynamic_unregister(LTT_FACILITY_SYMBOL);\n");
  fprintf(fp, "\tif(err != 0)\n");
  fprintf(fp, "\t\tprintk(KERN_ERR \"LTT : Error in unregistering facility.\\n\");\n");
  fprintf(fp, "\n");
  fprintf(fp, "}\n");
  fprintf(fp, "\n");
  fprintf(fp, "module_init(facility_init)\n");
  fprintf(fp, "module_exit(facility_exit)\n");
  fprintf(fp, "\n");
  fprintf(fp, "\n");
  fprintf(fp, "MODULE_LICENSE(\"GPL\");\n");
  fprintf(fp, "MODULE_AUTHOR(\"Mathieu Desnoyers\");\n");
  fprintf(fp, "MODULE_DESCRIPTION(\"Linux Trace Toolkit Facility\");\n");
  fprintf(fp, "\n");
  fprintf(fp, "#endif //MODULE\n");
  fprintf(fp, "\n");
  fprintf(fp, "#endif //CONFIG_LTT\n");
}


