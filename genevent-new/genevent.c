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

#define _GNU_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "genevent.h"
#include "parser.h"


#define TRUE 1
#define FALSE (!TRUE)

/* Debugging printf */
#ifdef DEBUG
#define dprintf(...) \
	do {\
		printf(__FILE__ ",%u,%s: ",\
				__LINE__, __func__);\
		printf(__VA_ARGS__);\
	} while(0)
#else
#define dprintf(...)
#endif


/* Code printing */

void print_tabs(unsigned int tabs, FILE *fd)
{
	for(unsigned int i = 0; i<tabs;i++)
		fprintf(fd, "\t");
}

/* Type size checking */
/* Uses #error in the generated code to signal error and stop the compiler */
int print_check(int fd);


/* Print types */
int print_types(int fd);


/* Print events */
int print_events(int fd);


/* print type.
 *
 * Copied from construct_types_and_fields in LTTV facility.c */

int print_type(type_descriptor_t * td, FILE *fd, unsigned int tabs,
		char *nest_name, char *field_name)
{
	char basename[PATH_MAX];
	unsigned int basename_len = 0;

	strcpy(basename, nest_name);
	basename_len = strlen(basename);
	
	/* For a named type, we use the type_name directly */
	if(td->type_name != NULL) {
		strncpy(basename, td->type_name, PATH_MAX);
		basename_len = strlen(basename);
	} else {
		/* For a unnamed type, there must be a field name */
		if((basename_len != 0)
				&& (basename[basename_len-1] != '_')
				&& (field_name[0] != '\0')) {
			strncat(basename, "_", PATH_MAX - basename_len);
			basename_len = strlen(basename);
		}
		strncat(basename, field_name, PATH_MAX - basename_len);
	}

	switch(td->type) {
		case INT_FIXED:
			fprintf(fd, "%s", intOutputTypes[getSizeindex(td->size)]);
			break;
		case UINT_FIXED:
			fprintf(fd, "%s", uintOutputTypes[getSizeindex(td->size)]);
			break;
		case CHAR:
			fprintf(fd, "signed char");
			break;
		case UCHAR:
			fprintf(fd, "unsigned char");
			break;
		case SHORT:
			fprintf(fd, "short");
			break;
		case USHORT:
			fprintf(fd, "unsigned short");
			break;
		case INT:
			fprintf(fd, "int");
			break;
		case UINT:
			fprintf(fd, "unsigned int");
			break;
		case FLOAT:
			fprintf(fd, "%s", floatOutputTypes[getSizeindex(td->size)]);
			break;
		case POINTER:
			fprintf(fd, "void *");
			break;
		case LONG:
			fprintf(fd, "long");
			break;
		case ULONG:
			fprintf(fd, "unsigned long");
			break;
		case SIZE_T:
			fprintf(fd, "size_t");
			break;
		case SSIZE_T:
			fprintf(fd, "ssize_t");
			break;
		case OFF_T:
			fprintf(fd, "off_t");
			break;
		case STRING:
			fprintf(fd, "char *");
			break;
		case ENUM:
			fprintf(fd, "enum lttng_%s", basename);
			break;
		case ARRAY:
			fprintf(fd, "lttng_array_%s", basename);
			break;
		case SEQUENCE:
			fprintf(fd, "lttng_sequence_%s", basename);
			break;
	case STRUCT:
			fprintf(fd, "struct lttng_%s", basename);
			break;
	case UNION:
			fprintf(fd, "union lttng_%s", basename);
			break;
	default:
	 	 	printf("print_type : unknown type\n");
			return 1;
	}

	return 0;
}

/* print type declaration.
 *
 * Copied from construct_types_and_fields in LTTV facility.c */

int print_type_declaration(type_descriptor_t * td, FILE *fd, unsigned int tabs,
		char *nest_name, char *field_name)
{
	char basename[PATH_MAX];
	unsigned int basename_len = 0;
	
	strncpy(basename, nest_name, PATH_MAX);
	basename_len = strlen(basename);
	
	/* For a named type, we use the type_name directly */
	if(td->type_name != NULL) {
		strncpy(basename, td->type_name, PATH_MAX);
		basename_len = strlen(basename);
	} else {
		/* For a unnamed type, there must be a field name, except for
		 * the array. */
		if((basename_len != 0)
				&& (basename[basename_len-1] != '_'
				&& (field_name[0] != '\0'))) {
			strncat(basename, "_", PATH_MAX - basename_len);
			basename_len = strlen(basename);
		}
		strncat(basename, field_name, PATH_MAX - basename_len);
	}

	switch(td->type) {
		case ENUM:
			fprintf(fd, "enum lttng_%s", basename);
			fprintf(fd, " {\n");
			for(unsigned int i=0;i<td->labels.position;i++){
				print_tabs(1, fd);
				fprintf(fd, "LTTNG_%s", ((char*)(td->labels.array[i])));
				fprintf(fd, ",\n");
			}
			fprintf(fd, "};\n");
			fprintf(fd, "\n");
			break;

		case ARRAY:
			dprintf("%s\n", basename);
			assert(td->size >= 0);
			if(td->nested_type->type_name == NULL) {
				/* Not a named nested type : we must print its declaration first */
				if(print_type_declaration(td->nested_type,
																	fd,	0, basename, "")) return 1;
			}
			fprintf(fd, "#define LTTNG_ARRAY_SIZE_%s %llu\n", basename,
					td->size);
			if(print_type(td->nested_type, fd, tabs, basename, "")) return 1;
			fprintf(fd, " lttng_array_%s[LTTNG_ARRAY_SIZE_%s];\n", basename,
					basename);
			fprintf(fd, "\n");
			break;
		case SEQUENCE:
			if(td->nested_type->type_name == NULL) {
				/* Not a named nested type : we must print its declaration first */
				if(print_type_declaration(td->nested_type,
																	fd,	0, basename, "")) return 1;
			}
			fprintf(fd, "typedef struct lttng_sequence_%s lttng_sequence_%s;\n",
					basename,
					basename);
			fprintf(fd, "struct lttng_sequence_%s", basename);
			fprintf(fd, " {\n");
			print_tabs(1, fd);
			fprintf(fd, "unsigned int len;\n");
			print_tabs(1, fd);
			if(print_type(td->nested_type, fd, tabs, basename, "")) return 1;
			fprintf(fd, " *array;\n");
			fprintf(fd, "};\n");
			fprintf(fd, "\n");
		break;

	case STRUCT:
			for(unsigned int i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				if(type->type_name == NULL) {
					/* Not a named nested type : we must print its declaration first */
					if(print_type_declaration(type,
																		fd,	0, basename, field->name)) return 1;
				}
			}
			fprintf(fd, "struct lttng_%s", basename);
			fprintf(fd, " {\n");
			for(unsigned int i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				print_tabs(1, fd);
				if(print_type(type, fd, tabs, basename, field->name)) return 1;
				fprintf(fd, " ");
				fprintf(fd, "%s", field->name);
				fprintf(fd, ";\n");
			}
			fprintf(fd, "};\n");
			fprintf(fd, "\n");
			break;
	case UNION:
			/* TODO : Do not allow variable length fields in a union */
			for(unsigned int i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				if(type->type_name == NULL) {
					/* Not a named nested type : we must print its declaration first */
					if(print_type_declaration(type,
																		fd,	0, basename, field->name)) return 1;
				}
			}
			fprintf(fd, "union lttng_%s", basename);
			fprintf(fd, " {\n");
			for(unsigned i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				print_tabs(1, fd);
				if(print_type(type, fd, tabs, basename, field->name)) return 1;
				fprintf(fd, " ");
				fprintf(fd, "%s", field->name);
				fprintf(fd, ";\n");
			}
			fprintf(fd, "};\n");
			fprintf(fd, "\n");
			break;
	default:
		dprintf("print_type_declaration : unknown type or nothing to declare.\n");
		break;
	}

	return 0;
}

/* Print the logging function of an event. This is the core of genevent */
int print_event_logging_function(char *basename, event_t *event, FILE *fd)
{
	fprintf(fd, "static inline void trace_%s(\n", basename);
	for(unsigned int j = 0; j < event->fields.position; j++) {
		/* For each field, print the function argument */
		field_t *f = (field_t*)event->fields.array[j];
		type_descriptor_t *t = f->type;
		print_tabs(2, fd);
		if(print_type(t, fd, 0, basename, f->name)) return 1;
		fprintf(fd, " %s", f->name);
		if(j < event->fields.position-1) {
			fprintf(fd, ",");
			fprintf(fd, "\n");
		}
	}
	if(event->fields.position == 0) {
		print_tabs(2, fd);
		fprintf(fd, "void");
	}
	fprintf(fd,")\n");
	fprintf(fd, "#ifndef CONFIG_LTT\n");
	fprintf(fd, "{\n");
	fprintf(fd, "}\n");
	fprintf(fd,"#else\n");
	fprintf(fd, "{\n");


	fprintf(fd, "}\n");
	fprintf(fd, "#endif //CONFIG_LTT\n\n");
	return 0;
}


/* ltt-facility-name.h : main logging header.
 * log_header */

void print_log_header_head(facility_t *fac, FILE *fd)
{
	fprintf(fd, "#ifndef _LTT_FACILITY_%s_H_\n", fac->capname);
	fprintf(fd, "#define _LTT_FACILITY_%s_H_\n\n", fac->capname);
	fprintf(fd, "\n");
	fprintf(fd, "/* Facility activation at compile time. */\n");
	fprintf(fd, "#ifdef CONFIG_LTT_FACILITY_%s\n\n", fac->capname);
}



int print_log_header_types(facility_t *fac, FILE *fd)
{
	sequence_t *types = &fac->named_types.values;
	fprintf(fd, "/* Named types */\n");
	fprintf(fd, "\n");
	
	for(unsigned int i = 0; i < types->position; i++) {
		/* For each named type, print the definition */
		if((print_type_declaration(types->array[i], fd,
						0, "", ""))) return 1;
	}
	return 0;
}

int print_log_header_events(facility_t *fac, FILE *fd)
{
	sequence_t *events = &fac->events;
	char basename[PATH_MAX];
	unsigned int facname_len;
	
	strncpy(basename, fac->name, PATH_MAX);
	facname_len = strlen(basename);
	strncat(basename, "_", PATH_MAX-facname_len);
	facname_len = strlen(basename);

	for(unsigned int i = 0; i < events->position; i++) {
		event_t *event = (event_t*)events->array[i];
		strncpy(&basename[facname_len], event->name, PATH_MAX-facname_len);
		
		/* For each event, print structure, and then logging function */
		fprintf(fd, "/* Event %s structures */\n",
				event->name);
		for(unsigned int j = 0; j < event->fields.position; j++) {
			/* For each unnamed type, print the definition */
			field_t *f = (field_t*)event->fields.array[j];
			type_descriptor_t *t = f->type;
			if(t->type_name == NULL)
				if((print_type_declaration(t, fd, 0, basename, f->name))) return 1;
		}
		fprintf(fd, "\n");

		fprintf(fd, "/* Event %s logging function */\n",
				event->name);

		if(print_event_logging_function(basename, event, fd)) return 1;

		fprintf(fd, "\n");
	}
	
	return 0;
}


void print_log_header_tail(facility_t *fac, FILE *fd)
{
	fprintf(fd, "#endif //CONFIG_LTT_FACILITY_%s\n\n", fac->capname);
	fprintf(fd, "#endif //_LTT_FACILITY_%s_H_\n",fac->capname);
}
	
int print_log_header(facility_t *fac)
{
	char filename[PATH_MAX];
	unsigned int filename_size = 0;
	FILE *fd;
	dprintf("%s\n", fac->name);

	strcpy(filename, "ltt-facility-");
	filename_size = strlen(filename);
	
	strncat(filename, fac->name, PATH_MAX - filename_size);
	filename_size = strlen(filename);

	strncat(filename, ".h", PATH_MAX - filename_size);
	filename_size = strlen(filename);
	

	fd = fopen(filename, "w");
	if(fd == NULL) {
		printf("Error opening file %s for writing : %s\n",
				filename, strerror(errno));
		return errno;
	}

	/* Print file head */
	print_log_header_head(fac, fd);

	/* print named types in declaration order */
	if(print_log_header_types(fac, fd)) return 1;

	/* Print events */
	if(print_log_header_events(fac, fd)) return 1;
	
	/* Print file tail */
	print_log_header_tail(fac, fd);

	
	fclose(fd);
	
	return 0;
}


/* ltt-facility-id-name.h : facility id.
 * log_id_header */
int print_id_header(facility_t *fac)
{
	char filename[PATH_MAX];
	unsigned int filename_size = 0;
	FILE *fd;
	dprintf("%s\n", fac->name);

	strcpy(filename, "ltt-facility-id-");
	filename_size = strlen(filename);
	
	strncat(filename, fac->name, PATH_MAX - filename_size);
	filename_size = strlen(filename);

	strncat(filename, ".h", PATH_MAX - filename_size);
	filename_size = strlen(filename);
	

	fd = fopen(filename, "w");
	if(fd == NULL) {
		printf("Error opening file %s for writing : %s\n",
				filename, strerror(errno));
		return errno;
	}

	fclose(fd);

	return 0;
}


/* ltt-facility-loader-name.h : facility specific loader info.
 * loader_header */
int print_loader_header(facility_t *fac)
{
	char filename[PATH_MAX];
	unsigned int filename_size = 0;
	FILE *fd;
	dprintf("%s\n", fac->name);

	strcpy(filename, "ltt-facility-loader-");
	filename_size = strlen(filename);
	
	strncat(filename, fac->name, PATH_MAX - filename_size);
	filename_size = strlen(filename);

	strncat(filename, ".h", PATH_MAX - filename_size);
	filename_size = strlen(filename);
	

	fd = fopen(filename, "w");
	if(fd == NULL) {
		printf("Error opening file %s for writing : %s\n",
				filename, strerror(errno));
		return errno;
	}

	fclose(fd);

	return 0;
}

/* ltt-facility-loader-name.c : generic faciilty loader
 * loader_c */
int print_loader_c(facility_t *fac)
{
	char filename[PATH_MAX];
	unsigned int filename_size = 0;
	FILE *fd;
	dprintf("%s\n", fac->name);

	strcpy(filename, "ltt-facility-loader-");
	filename_size = strlen(filename);
	
	strncat(filename, fac->name, PATH_MAX - filename_size);
	filename_size = strlen(filename);

	strncat(filename, ".c", PATH_MAX - filename_size);
	filename_size = strlen(filename);
	

	fd = fopen(filename, "w");
	if(fd == NULL) {
		printf("Error opening file %s for writing : %s\n",
				filename, strerror(errno));
		return errno;
	}


	fclose(fd);

	return 0;
}



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


	/* ltt-facility-name.h : main logging header.
	 * log_header */
	err = print_log_header(fac);
	if(err)	return err;

	/* ltt-facility-id-name.h : facility id.
	 * log_id_header */
	err = print_id_header(fac);
	if(err)	return err;
	
	/* ltt-facility-loader-name.h : facility specific loader info.
	 * loader_header */
	err = print_loader_header(fac);
	if(err)	return err;

	/* ltt-facility-loader-name.c : generic faciilty loader
	 * loader_c */
	err = print_loader_c(fac);
	if(err)	return err;

	/* close the facility */
	ltt_facility_close(fac);
	
	return 0;
}


