/******************************************************************************
 * Genevent
 *
 * Event generator. XML to logging C code converter.
 *
 * Program parameters :
 * ./genevent name.xml
 *
 * Will generate ltt-facility-name.h, ltt-facility-id-name.h
 * ltt-facility-loader-name.c, ltt-facility-loader-name.h
 * in the current directory.
 * 
 * Supports : 
 * 	- C Alignment
 * 	- C types : struct, union, enum, basic types.
 * 	- Architectures : LP32, ILP32, ILP64, LLP64, LP64.
 *
 * Additionnal structures supported :
 *	- embedded variable size strings
 *	- embedded variable size arrays
 *	- embedded variable size sequences
 * 
 * Notes :
 * (1)
 * enums are limited to integer type, as this is what is used in C. Note,
 * however, that ISO/IEC 9899:TC2 specify that the type of enum can be char,
 * unsigned int or int. This is implementation defined (compiler). That's why we
 * add a check for sizeof enum.
 *
 * (2)
 * Because of archtecture defined type sizes, we need to ask for ltt_align
 * (which gives the alignment) by passing basic types, not their actual sizes.
 * It's up to ltt_align to determine sizes of types.
 *
 * Note that, from
 * http://www.usenix.org/publications/login/standards/10.data.html
 * (Andrew Josey <a.josey@opengroup.org>) :
 *
 *	Data Type	LP32	ILP32	ILP64	LLP64	LP64
 *	char	8	8	8	8	8
 *	short	16	16	16	16 	16
 *	int32 			32
 *	int	16	32	64	32	32
 *	long	32	32	64	32	64
 *	long long (int64)					64
 *	pointer	32	32	64	64	64
 *
 * With these constraints :
 * sizeof(char) <= sizeof(short) <= sizeof(int)
 *					<= sizeof(long) = sizeof(size_t)
 * 
 * and therefore sizeof(long) <= sizeof(pointer) <= sizeof(size_t)
 *
 * Which means we only have to remember which is the biggest type in a structure
 * to know the structure's alignment.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "genevent.h"
#include "parser.h"


#define TRUE 1
#define FALSE (!TRUE)

/* Code printing */

/* Type size checking */
int print_check(int fd);


/* Print types */
int print_types(int fd);


/* Print events */
int print_events(int fd);


/* open facility */
/* code taken from ltt_facility_open in ltt/facility.c in lttv */

/*****************************************************************************
 *Function name
 *		ltt_facility_open			 : open facilities
 *Input params
 *		pathname								: the path name of the facility	 
 *
 *	Open the facility corresponding to the right checksum.
 * 
 *returns the facility on success, NULL on error.
 ****************************************************************************/
facility_t *ltt_facility_open(char * pathname)
{
	int ret = 0;
	char *token;
	parse_file_t in;
	facility_t * fac = NULL;
	unsigned long checksum;
	char buffer[BUFFER_SIZE];
	int generated = FALSE;

	in.buffer = &(buffer[0]);
	in.lineno = 0;
	in.error = error_callback;
	in.name = pathname;
	in.unget = 0;

	in.fp = fopen(in.name, "r");
	if(in.fp == NULL) {
		ret = 1;
		goto open_error;
	}

	while(1){
		token = getToken(&in);
		if(in.type == ENDFILE) break;

		if(generated) {
			printf("More than one facility in the file. Only using the first one.\n");
			break;
		}
		
		if(strcmp(token, "<")) in.error(&in,"not a facility file");
		token = getName(&in);

		if(strcmp("facility",token) == 0) {
			fac = malloc(sizeof(facility_t));
			fac->name = NULL;
			fac->description = NULL;
			sequence_init(&(fac->events));
			table_init(&(fac->named_types));
			sequence_init(&(fac->unnamed_types));
			
			parseFacility(&in, fac);

			//check if any namedType is not defined
			checkNamedTypesImplemented(&fac->named_types);
		
			generateChecksum(fac->name, &checksum, &fac->events);
	
			generated = TRUE;
		}
		else {
			printf("facility token was expected in file %s\n", in.name);
			ret = 1;
			goto parse_error;
		}
	}
	
 parse_error:
	fclose(in.fp);
open_error:

	if(!generated) {
		printf("Cannot find facility %s\n", pathname);
		fac = NULL;
	}

	return fac;
}

/* Close the facility */
void ltt_facility_close(facility_t *fac)
{
	free(fac->name);
	free(fac->capname);
	free(fac->description);
	freeEvents(&fac->events);
	sequence_dispose(&fac->events);
	freeNamedType(&fac->named_types);
	table_dispose(&fac->named_types);
	freeTypes(&fac->unnamed_types);
	sequence_dispose(&fac->unnamed_types);			
	free(fac);
}


/* Show help */
void show_help(int argc, char ** argv)
{
	printf("Genevent help : \n");
	printf("\n");
	printf("Use %s name.xml\n", argv[0]);
	printf("to create :\n");
	printf("ltt-facility-name.h\n");
	printf("ltt-facility-id-name.h\n");
	printf("ltt-facility-loader-name.h\n");
	printf("ltt-facility-loader-name.c\n");
	printf("In the current directory.\n");
	printf("\n");
}

/* Parse program arguments */
/* Return values :
 * 0 : continue program
 * -1 : stop program, return 0
 * > 0 : stop program, return value as exit.
 */
int check_args(int argc, char **argv)
{
	if(argc < 2) {
		printf("Not enough arguments\n");
		show_help(argc, argv);
		return EINVAL;
	}
	
	if(strcmp(argv[1], "-h") == 0) {
		show_help(argc, argv);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int err = 0;
	facility_t *fac;
	
	err = check_args(argc, argv);
	if(err > 0) return err;
	else if(err < 0) return 0;

	/* open the facility */
	fac = ltt_facility_open(argv[1]);
	if(fac == NULL) {
		printf("Error opening file %s for reading : %s\n",
				argv[1], strerror(errno));
		return errno;
	}

	/* generate the output C files */


	/* ltt-facility-name.h : main logging header */
	

	/* ltt-facility-id-name.h : facility id. */
	

	/* ltt-facility-loader-name.h : facility specific loader info. */

	/* ltt-facility-loader-name.c : generic faciilty loader */



	/* close the facility */
	ltt_facility_close(fac);
	
	return 0;
}


