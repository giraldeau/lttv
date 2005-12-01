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

/* Print logging function argument */
int print_arg(type_descriptor_t * td, FILE *fd, unsigned int tabs,
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

	print_tabs(tabs, fd);

	switch(td->type) {
		case INT_FIXED:
			fprintf(fd, "%s", intOutputTypes[getSizeindex(td->size)]);
			fprintf(fd, " %s", field_name);
			break;
		case UINT_FIXED:
			fprintf(fd, "%s", uintOutputTypes[getSizeindex(td->size)]);
			fprintf(fd, " %s", field_name);
			break;
		case CHAR:
			fprintf(fd, "signed char");
			fprintf(fd, " %s", field_name);
			break;
		case UCHAR:
			fprintf(fd, "unsigned char");
			fprintf(fd, " %s", field_name);
			break;
		case SHORT:
			fprintf(fd, "short");
			fprintf(fd, " %s", field_name);
			break;
		case USHORT:
			fprintf(fd, "unsigned short");
			fprintf(fd, " %s", field_name);
			break;
		case INT:
			fprintf(fd, "int");
			fprintf(fd, " %s", field_name);
			break;
		case UINT:
			fprintf(fd, "unsigned int");
			fprintf(fd, " %s", field_name);
			break;
		case FLOAT:
			fprintf(fd, "%s", floatOutputTypes[getSizeindex(td->size)]);
			fprintf(fd, " %s", field_name);
			break;
		case POINTER:
			fprintf(fd, "void *");
			fprintf(fd, " %s", field_name);
			break;
		case LONG:
			fprintf(fd, "long");
			fprintf(fd, " %s", field_name);
			break;
		case ULONG:
			fprintf(fd, "unsigned long");
			fprintf(fd, " %s", field_name);
			break;
		case SIZE_T:
			fprintf(fd, "size_t");
			fprintf(fd, " %s", field_name);
			break;
		case SSIZE_T:
			fprintf(fd, "ssize_t");
			fprintf(fd, " %s", field_name);
			break;
		case OFF_T:
			fprintf(fd, "off_t");
			fprintf(fd, " %s", field_name);
			break;
		case STRING:
			fprintf(fd, "char *");
			fprintf(fd, " %s", field_name);
			break;
		case ENUM:
			fprintf(fd, "enum lttng_%s", basename);
			fprintf(fd, " %s", field_name);
			break;
		case ARRAY:
			fprintf(fd, "lttng_array_%s", basename);
			fprintf(fd, " %s", field_name);
			break;
		case SEQUENCE:
			fprintf(fd, "lttng_sequence_%s *", basename);
			fprintf(fd, " %s", field_name);
			break;
	case STRUCT:
			fprintf(fd, "struct lttng_%s *", basename);
			fprintf(fd, " %s", field_name);
			break;
	case UNION:
			fprintf(fd, "union lttng_%s *", basename);
			fprintf(fd, " %s", field_name);
			break;
	default:
	 	 	printf("print_type : unknown type\n");
			return 1;
	}

	return 0;
}


/* Does the type has a fixed size ? (as know from the compiler)
 *
 * 1 : fixed size
 * 0 : variable length
 */
int has_type_fixed_size(type_descriptor_t *td)
{
	switch(td->type) {
		case INT_FIXED:
		case UINT_FIXED:
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
		case INT:
		case UINT:
		case FLOAT:
		case POINTER:
		case LONG:
		case ULONG:
		case SIZE_T:
		case SSIZE_T:
		case OFF_T:
		case ENUM:
		case UNION: /* The union must have fixed size children. Must be checked by
									 the parser */
			return 1;
			break;
		case STRING:
		case SEQUENCE:
			return 0;
			break;
		case STRUCT:
			{
				int has_type_fixed = 0;
				for(unsigned int i=0;i<td->fields.position;i++){
					field_t *field = (field_t*)(td->fields.array[i]);
					type_descriptor_t *type = field->type;
					
					has_type_fixed = has_type_fixed_size(type);
					if(!has_type_fixed) return 0;
				}
				return 1;
			}
			break;
		case ARRAY:
			assert(td->size >= 0);
			return has_type_fixed_size(((field_t*)td->fields.array[0])->type);
			break;
		case NONE:
			printf("There is a type defined to NONE : bad.\n");
			assert(0);
			break;
	}
	return 0; //make gcc happy.
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
			if(((field_t*)td->fields.array[0])->type->type_name == NULL) {
				/* Not a named nested type : we must print its declaration first */
				if(print_type_declaration(((field_t*)td->fields.array[0])->type,
																	fd,	0, basename, "")) return 1;
			}
			fprintf(fd, "#define LTTNG_ARRAY_SIZE_%s %zu\n", basename,
					td->size);
			fprintf(fd, "typedef ");
			if(print_type(((field_t*)td->fields.array[0])->type,
						fd, tabs, basename, "")) return 1;
			fprintf(fd, " lttng_array_%s[LTTNG_ARRAY_SIZE_%s];\n", basename,
					basename);
			fprintf(fd, "\n");
			break;
		case SEQUENCE:
			/* We assume that the sequence length type does not need to be declared.
			 */
			if(((field_t*)td->fields.array[1])->type->type_name == NULL) {
				/* Not a named nested type : we must print its declaration first */
				if(print_type_declaration(((field_t*)td->fields.array[1])->type,
																	fd,	0, basename, "")) return 1;
			}
			fprintf(fd, "typedef struct lttng_sequence_%s lttng_sequence_%s;\n",
					basename,
					basename);
			fprintf(fd, "struct lttng_sequence_%s", basename);
			fprintf(fd, " {\n");
			print_tabs(1, fd);
			if(print_type(((field_t*)td->fields.array[0])->type,
						fd, tabs, basename, "")) return 1;
			fprintf(fd, "len;\n");
			print_tabs(1, fd);
			if(print_type(((field_t*)td->fields.array[1])->type,
						fd, tabs, basename, "")) return 1;
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


/* print type alignment.
 *
 * Copied from construct_types_and_fields in LTTV facility.c
 *
 * basename is the name which identifies the type (along with a prefix
 * (possibly)). */

int print_type_alignment(type_descriptor_t * td, FILE *fd, unsigned int tabs,
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
				&& field_name != NULL
				&& (field_name[0] != '\0'))) {
			strncat(basename, "_", PATH_MAX - basename_len);
			basename_len = strlen(basename);
		}
		if(field_name != NULL)
			strncat(basename, field_name, PATH_MAX - basename_len);
	}
	
	switch(td->type) {
		case INT_FIXED:
		case UINT_FIXED:
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
		case INT:
		case UINT:
		case FLOAT:
		case POINTER:
		case LONG:
		case ULONG:
		case SIZE_T:
		case SSIZE_T:
		case OFF_T:
		case ENUM:
			fprintf(fd, "sizeof(");
			if(print_type(td, fd, 0, basename, "")) return 1;
			fprintf(fd, ")");
			break;
		case STRING:
			fprintf(fd, "sizeof(char)");
			break;
		case SEQUENCE:
			fprintf(fd, "lttng_get_alignment_sequence_%s(&obj->%s)", basename,
					field_name);
			break;
		case STRUCT:
			fprintf(fd, "lttng_get_alignment_struct_%s(&obj->%s)", basename,
					field_name);
			break;
		case UNION:
			fprintf(fd, "lttng_get_alignment_union_%s(&obj->%s)", basename,
					field_name);
			break;
		case ARRAY:
			fprintf(fd, "lttng_get_alignment_array_%s(obj->%s)", basename,
					field_name);
			break;
		case NONE:
			printf("error : type NONE unexpected\n");
			return 1;
			break;
	}

	return 0;
}

/* print type write.
 *
 * Copied from construct_types_and_fields in LTTV facility.c
 *
 * basename is the name which identifies the type (along with a prefix
 * (possibly)). */

int print_type_write(type_descriptor_t * td, FILE *fd, unsigned int tabs,
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
		case INT_FIXED:
		case UINT_FIXED:
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
		case INT:
		case UINT:
		case FLOAT:
		case POINTER:
		case LONG:
		case ULONG:
		case SIZE_T:
		case SSIZE_T:
		case OFF_T:
		case ENUM:
			print_tabs(tabs, fd);
			fprintf(fd, "size = ");
			fprintf(fd, "sizeof(");
			if(print_type(td, fd, 0, basename, "")) return 1;
			fprintf(fd, ");\n");
			print_tabs(tabs, fd);
			fprintf(fd, "size += ltt_align(*to+*len, size) + size;\n");
			print_tabs(tabs, fd);
			fprintf(fd, "*len += size;");
			break;
		case STRING:
			print_tabs(tabs, fd);
			fprintf(fd, "lttng_write_string_%s(buffer, to_base, to, from, len, obj->%s);\n", basename, field_name);
			break;
		case SEQUENCE:
			print_tabs(tabs, fd);
			fprintf(fd, "lttng_write_%s(buffer, to_base, to, from, len, &obj->%s)", basename,
					field_name);
			break;
		case STRUCT:
			print_tabs(tabs, fd);
			fprintf(fd, "lttng_write_struct_%s(buffer, to_base, to, from, len, &obj->%s)", basename,
					field_name);
			break;
		case UNION:
			print_tabs(tabs, fd);
			fprintf(fd, "lttng_write_union_%s(buffer, to_base, to, from, len, &obj->%s)", basename,
					field_name);
			break;
		case ARRAY:
			print_tabs(tabs, fd);
			fprintf(fd, "lttng_write_array_%s(buffer, to_base, to, from, len, obj->%s)", basename,
					field_name);
			break;
		case NONE:
			printf("Error : type NONE unexpected\n");
			return 1;
			break;
	}

	return 0;
}



/* print type alignment function.
 *
 * Copied from construct_types_and_fields in LTTV facility.c
 *
 * basename is the name which identifies the type (along with a prefix
 * (possibly)). */

int print_type_alignment_fct(type_descriptor_t * td, FILE *fd,
		unsigned int tabs,
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
		case SEQUENCE:
			/* Function header */
			fprintf(fd, "static inline size_t lttng_get_alignment_sequence_%s(\n",
					basename);
			print_tabs(2, fd);
			if(print_type(td,	fd, 0, basename, "")) return 1;
			fprintf(fd, " *obj)\n");
			fprintf(fd, "{\n");
			print_tabs(1, fd);
			fprintf(fd, "size_t align=0, localign;");
			fprintf(fd, "\n");
			print_tabs(1, fd);
			fprintf(fd, "localign = ");
			if(print_type_alignment(((field_t*)td->fields.array[0])->type,
						fd, 0, basename, ((field_t*)td->fields.array[0])->name)) return 1;
			fprintf(fd, ";\n");
			print_tabs(1, fd);
			fprintf(fd, "align = max(align, localign);\n");
			fprintf(fd, "\n");
			print_tabs(1, fd);
			fprintf(fd, "localign = ");
			if(print_type_alignment(((field_t*)td->fields.array[1])->type,
						fd, 0, basename, ((field_t*)td->fields.array[1])->name)) return 1;
			fprintf(fd, ";\n");
			print_tabs(1, fd);
			fprintf(fd, "align = max(align, localign);\n");
			fprintf(fd, "\n");
			print_tabs(1, fd);
			fprintf(fd, "return align;\n");
			break;
		case STRUCT:
			/* Function header */
			fprintf(fd, "static inline size_t lttng_get_alignment_struct_%s(\n",
					basename);
			print_tabs(2, fd);
			if(print_type(td,	fd, 0, basename, "")) return 1;
			fprintf(fd, " *obj)\n");
			fprintf(fd, "{\n");
			print_tabs(1, fd);
			fprintf(fd, "size_t align=0, localign;");
			fprintf(fd, "\n");
			for(unsigned int i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				print_tabs(1, fd);
				fprintf(fd, "localign = ");
				if(print_type_alignment(type, fd, 0, basename, field->name)) return 1;
				fprintf(fd, ";\n");
				print_tabs(1, fd);
				fprintf(fd, "align = max(align, localign);\n");
				fprintf(fd, "\n");
			}
			print_tabs(1, fd);
			fprintf(fd, "return align;\n");

			break;
		case UNION:
			/* Function header */
			fprintf(fd, "static inline size_t lttng_get_alignment_union_%s(\n",
					basename);
			print_tabs(2, fd);
			if(print_type(td,	fd, 0, basename, "")) return 1;
			fprintf(fd, " *obj)\n");
			fprintf(fd, "{\n");
			print_tabs(1, fd);
			fprintf(fd, "size_t align=0, localign;");
			fprintf(fd, "\n");
			for(unsigned int i=0;i<td->fields.position;i++){
				field_t *field = (field_t*)(td->fields.array[i]);
				type_descriptor_t *type = field->type;
				print_tabs(1, fd);
				fprintf(fd, "localign = ");
				if(print_type_alignment(type, fd, 0, basename, field->name)) return 1;
				fprintf(fd, ";\n");
				print_tabs(1, fd);
				fprintf(fd, "align = max(align, localign);\n");
				fprintf(fd, "\n");
			}
			print_tabs(1, fd);
			fprintf(fd, "return align;\n");

			break;
		case ARRAY:
			/* Function header */
			fprintf(fd, "static inline size_t lttng_get_alignment_array_%s(\n",
					basename);
			print_tabs(2, fd);
			if(print_type(td,	fd, 0, basename, "")) return 1;
			fprintf(fd, " obj)\n");
			fprintf(fd, "{\n");
			print_tabs(1, fd);
			fprintf(fd, "return \n");
			if(print_type_alignment(((field_t*)td->fields.array[0])->type,
						fd, 0, basename, ((field_t*)td->fields.array[0])->name)) return 1;
			fprintf(fd, ";\n");
			break;
		default:
			printf("print_type_alignment_fct : type has no alignment function.\n");
			return 0;
			break;
	}


	/* Function footer */
	fprintf(fd, "}\n");
	fprintf(fd, "\n");

	return 0;
}

/* print type write function.
 *
 * Copied from construct_types_and_fields in LTTV facility.c
 *
 * basename is the name which identifies the type (along with a prefix
 * (possibly)). */

int print_type_write_fct(type_descriptor_t * td, FILE *fd, unsigned int tabs,
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
		case SEQUENCE:
		case STRUCT:
		case UNION:
		case ARRAY:
			break;
		default:
			printf("print_type_write_fct : type has no write function.\n");
			return 0;
			break;
	}
	
	/* Print header */
	switch(td->type) {
		case SEQUENCE:
			fprintf(fd, "static inline void lttng_write_sequence_%s(\n",
					basename);
			break;
		case STRUCT:
			fprintf(fd, "static inline void lttng_write_struct_%s(\n", basename,
					field_name);
			break;
		case UNION:
			fprintf(fd, "static inline void lttng_write_union_%s(\n", basename,
					field_name);
			break;
		case ARRAY:
			fprintf(fd, "static inline void lttng_write_array_%s(\n", basename,
					field_name);
			break;
		default:
			printf("print_type_write_fct : type has no write function.\n");
			break;
	}

	print_tabs(2, fd);
	fprintf(fd, "void *buffer,\n");
	print_tabs(2, fd);
	fprintf(fd, "size_t *to_base,\n");
	print_tabs(2, fd);
	fprintf(fd, "size_t *to,\n");
	print_tabs(2, fd);
	fprintf(fd, "void **from,\n");
	print_tabs(2, fd);
	fprintf(fd, "size_t *len,\n");
	print_tabs(2, fd);
	if(print_type(td,	fd, 0, basename, "")) return 1;

	switch(td->type) {
		case SEQUENCE:
			fprintf(fd, " *obj)\n");
			break;
		case STRUCT:
			fprintf(fd, " *obj)\n");
			break;
		case UNION:
			fprintf(fd, " *obj)\n");
			break;
		case ARRAY:
			fprintf(fd, " obj)\n");
			break;
		default:
			printf("print_type_write_fct : type has no write function.\n");
			break;
	}

	fprintf(fd, "{\n");
	print_tabs(1, fd);
	fprintf(fd, "size_t align, size;\n");
	fprintf(fd, "\n");

	switch(td->type) {
		case SEQUENCE:
		case STRING:
			print_tabs(1, fd);
			fprintf(fd, "/* Flush pending memcpy */\n");
			print_tabs(1, fd);
			fprintf(fd, "if(*len != 0) {\n");
			print_tabs(2, fd);
			fprintf(fd, "if(buffer != NULL)\n");
			print_tabs(3, fd);
			fprintf(fd, "memcpy(buffer+*to_base+*to, *from, *len);\n");
			print_tabs(1, fd);
			fprintf(fd, "}\n");
			print_tabs(1, fd);
			fprintf(fd, "*to += *len;\n");
			print_tabs(1, fd);
			fprintf(fd, "*len = 0;\n");
			fprintf(fd, "\n");
			break;
		case STRUCT:
		case UNION:
		case ARRAY:
			break;
		default:
			printf("print_type_write_fct : type has no write function.\n");
			break;
	}
	
	print_tabs(1, fd);
	fprintf(fd, "align = ");
	if(print_type_alignment(td, fd, 0, basename, field_name)) return 1;
	fprintf(fd, ";\n");
	fprintf(fd, "\n");
	print_tabs(1, fd);
	fprintf(fd, "if(*len == 0) {\n");
	print_tabs(2, fd);
	fprintf(fd, "*to += ltt_align(*to, align); /* align output */\n");
	print_tabs(1, fd);
	fprintf(fd, "} else {\n");
	print_tabs(2, fd);
	fprintf(fd, "*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */\n");
	print_tabs(1, fd);
	fprintf(fd, "}\n");
	fprintf(fd, "\n");

	/* First, check if the type has a fixed size. If it is the case, then the size
	 * to write is know by the compiler : simply use a sizeof() */
	if(has_type_fixed_size(td)) {
		print_tabs(1, fd);
		fprintf(fd, "/* Contains only fixed size fields : use compiler sizeof() */\n");
		fprintf(fd, "\n");
		print_tabs(1, fd);
		fprintf(fd, "*len += sizeof(");
		if(print_type(td, fd, 0, basename, field_name)) return 1;
		fprintf(fd, ");\n");
	} else {
		/* The type contains nested variable size subtypes :
		 * we must write field by field. */
		print_tabs(1, fd);
		fprintf(fd, "/* Contains variable sized fields : must explode the structure */\n");
		fprintf(fd, "\n");

		switch(td->type) {
			case SEQUENCE:
				print_tabs(1, fd);
				fprintf(fd, "/* Copy members */\n");
				print_tabs(1, fd);
				fprintf(fd, "size = sizeof(\n");
				if(print_type_write(((field_t*)td->fields.array[0])->type,
						fd, 1, basename, ((field_t*)td->fields.array[0])->name)) return 1;
				fprintf(fd, ");\n");
				print_tabs(1, fd);
				fprintf(fd, "*to += ltt_align(*to, size);\n");
				print_tabs(1, fd);
				fprintf(fd, "if(buffer != NULL)\n");
				print_tabs(2, fd);
				fprintf(fd, "memcpy(buffer+*to_base+*to, &obj->len, size);\n");
				print_tabs(1, fd);
				fprintf(fd, "*to += size;\n");
				fprintf(fd, "\n");
				
				/* Write the child : varlen child or not ? */
				if(has_type_fixed_size(((field_t*)td->fields.array[1])->type)) {
					/* Fixed size len child : use a multiplication of its size */
					print_tabs(1, fd);
					fprintf(fd, "size = sizeof(\n");
					if(print_type_write(((field_t*)td->fields.array[1])->type,
							fd, 1, basename, ((field_t*)td->fields.array[1])->name)) return 1;
					fprintf(fd, ");\n");
					print_tabs(1, fd);
					fprintf(fd, "*to += ltt_align(*to, size);\n");
					print_tabs(1, fd);
					fprintf(fd, "size = obj->len * size;\n");
					print_tabs(1, fd);
					fprintf(fd, "if(buffer != NULL)\n");
					print_tabs(2, fd);
					fprintf(fd, "memcpy(buffer+*to_base+*to, obj->array, size);\n");
					print_tabs(1, fd);
					fprintf(fd, "*to += size;\n");
					fprintf(fd, "\n");
				} else {
					print_tabs(1, fd);
					fprintf(fd, "/* Variable length child : iter. */\n");
					print_tabs(1, fd);
					fprintf(fd, "for(unsigned int i=0; i<obj->len; i++) {\n");
					if(print_type_write(((field_t*)td->fields.array[1])->type,
							fd, 2, basename, ((field_t*)td->fields.array[1])->name)) return 1;
					print_tabs(1, fd);
					fprintf(fd, "}\n");
				}
				fprintf(fd, "\n");
				print_tabs(1, fd);
				fprintf(fd, "/* Realign the *to_base on arch size, set *to to 0 */\n");
				print_tabs(1, fd);
				fprintf(fd, "*to = ltt_align(*to, sizeof(void *));\n");
				print_tabs(1, fd);
				fprintf(fd, "*to_base = *to_base+*to;\n");
				print_tabs(1, fd);
				fprintf(fd, "*to = 0;\n");
				fprintf(fd, "\n");
				fprintf(fd, "/* Put source *from just after the C sequence */\n");
				print_tabs(1, fd);
				fprintf(fd, "*from = obj+1;\n");
				break;
			case STRING:
				fprintf(fd, "size = strlen(obj);\n");
				print_tabs(1, fd);
				fprintf(fd, "if(buffer != NULL)\n");
				print_tabs(2, fd);
				fprintf(fd, "memcpy(buffer+*to_base+*to, obj, size);\n");
				print_tabs(1, fd);
				fprintf(fd, "*to += size;\n");
				fprintf(fd, "\n");
				print_tabs(1, fd);
				fprintf(fd, "/* Realign the *to_base on arch size, set *to to 0 */\n");
				print_tabs(1, fd);
				fprintf(fd, "*to = ltt_align(*to, sizeof(void *));\n");
				print_tabs(1, fd);
				fprintf(fd, "*to_base = *to_base+*to;\n");
				print_tabs(1, fd);
				fprintf(fd, "*to = 0;\n");
				fprintf(fd, "\n");
				fprintf(fd, "/* Put source *from just after the C sequence */\n");
				print_tabs(1, fd);
				fprintf(fd, "*from = obj+1;\n");
				break;
			case STRUCT:
				for(unsigned int i=0;i<td->fields.position;i++){
					field_t *field = (field_t*)(td->fields.array[i]);
					type_descriptor_t *type = field->type;
					if(print_type_write(type,
							fd, 1, basename, field->name)) return 1;
					fprintf(fd, "\n");
				}
				break;
			case UNION:
				printf("ERROR : A union CANNOT contain a variable size child.\n");
				return 1;
				break;
			case ARRAY:
				/* Write the child : varlen child or not ? */
				if(has_type_fixed_size(((field_t*)td->fields.array[0])->type)) {
					/* Error : if an array has a variable size, then its child must also
					 * have a variable size. */
					assert(0);
				} else {
					print_tabs(1, fd);
					fprintf(fd, "/* Variable length child : iter. */\n");
					print_tabs(1, fd);
					fprintf(fd, "for(unsigned int i=0; i<LTTNG_ARRAY_SIZE_%s; i++) {\n", basename);
					if(print_type_write(((field_t*)td->fields.array[1])->type,
							fd, 2, basename, ((field_t*)td->fields.array[1])->name)) return 1;
					print_tabs(1, fd);
					fprintf(fd, "}\n");
				}
				break;
			default:
				printf("print_type_write_fct : type has no write function.\n");
				break;
		}


	}


	/* Function footer */
	fprintf(fd, "}\n");
	fprintf(fd, "\n");
	return 0;
}



/* Print the logging function of an event. This is the core of genevent */
int print_event_logging_function(char *basename, facility_t *fac,
		event_t *event, FILE *fd)
{
	fprintf(fd, "static inline void trace_%s(\n", basename);
	for(unsigned int j = 0; j < event->fields.position; j++) {
		/* For each field, print the function argument */
		field_t *f = (field_t*)event->fields.array[j];
		type_descriptor_t *t = f->type;
		if(print_arg(t, fd, 2, basename, f->name)) return 1;
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
	/* Print the function variables */
	print_tabs(1, fd);
	fprintf(fd, "unsigned int index;\n");
	print_tabs(1, fd);
	fprintf(fd, "struct ltt_channel_struct *channel;\n");
	print_tabs(1, fd);
	fprintf(fd, "struct ltt_trace_struct *trace;\n");
	print_tabs(1, fd);
	fprintf(fd, "struct rchan_buf *relayfs_buf;\n");
	print_tabs(1, fd);
	fprintf(fd, "void *buffer = NULL;\n");
	print_tabs(1, fd);
	fprintf(fd, "size_t to_base = 0; /* The buffer is allocated on arch_size alignment */\n");
	print_tabs(1, fd);
	fprintf(fd, "size_t to = 0;\n");
	print_tabs(1, fd);
	fprintf(fd, "void *from;");
	print_tabs(1, fd);
	fprintf(fd, "size_t len = 0;\n");
	print_tabs(1, fd);
	fprintf(fd, "size_t slot_size;\n");
	print_tabs(1, fd);
	fprintf(fd, "cycles_t tsc;\n");
	print_tabs(1, fd);
	fprintf(fd, "size_t before_hdr_pad, size_t after_hdr_pad;\n");
	fprintf(fd, "\n");
	
	print_tabs(1, fd);
	fprintf(fd, "if(ltt_traces.num_active_traces == 0) return;\n");
	fprintf(fd, "\n");

	/* Calculate event variable len + event data alignment offset.
	 * Assume that the padding for alignment starts at a void*
	 * address.
	 * This excludes the header size and alignment. */

	print_tabs(1, fd);
	fprintf(fd, "/* For each field, calculate the field size. */\n");
	print_tabs(1, fd);
	fprintf(fd, "/* size = to_base + to + len */\n");
	print_tabs(1, fd);
	fprintf(fd, "/* Assume that the padding for alignment starts at a\n");
	print_tabs(1, fd);
	fprintf(fd, " * sizeof(void *) address. */\n");
	fprintf(fd, "\n");

	for(unsigned int i=0;i<event->fields.position;i++){
		field_t *field = (field_t*)(event->fields.array[i]);
		type_descriptor_t *type = field->type;
		if(print_type_write(type,
				fd, 1, basename, field->name)) return 1;
		fprintf(fd, "\n");
	}
	fprintf(fd, "\n");

	/* Take locks : make sure the trace does not vanish while we write on
	 * it. A simple preemption disabling is enough (using rcu traces). */
	print_tabs(1, fd);
	fprintf(fd, "preempt_disable();\n");
	print_tabs(1, fd);
	fprintf(fd, "ltt_nesting[smp_processor_id()]++;\n");

	/* Get facility index */

	if(event->per_tracefile) {
		print_tabs(1, fd);
		fprintf(fd, "index = tracefile_index;\n");
	} else {
		print_tabs(1, fd);
		fprintf(fd, 
			"index = ltt_get_index_from_facility(ltt_facility_%s_%X,\n"\
					"\t\t\t\t\t\tevent_%s);\n",
				fac->name, fac->checksum, event->name);
	}
	fprintf(fd,"\n");

	
	/* For each trace */
	print_tabs(1, fd);
	fprintf(fd, "list_for_each_entry_rcu(trace, &ltt_traces.head, list) {\n");
	print_tabs(2, fd);
	fprintf(fd, "if(!trace->active) continue;\n\n");

	if(event->per_trace) {
		print_tabs(2, fd);
		fprintf(fd, "if(dest_trace != trace) continue;\n\n");
	}
 
	print_tabs(2, fd);
	fprintf(fd, "channel = ltt_get_channel_from_index(trace, index);\n");
	print_tabs(2, fd);
	fprintf(fd, "relayfs_buf = channel->rchan->buf[channel->rchan->buf->cpu];\n");
	fprintf(fd, "\n");

	
	/* Relay reserve */
	/* If error, increment event lost counter (done by ltt_reserve_slot) and 
	 * return */
	print_tabs(2, fd);
	fprintf(fd, "slot_size = 0;\n");
	print_tabs(2, fd);
	fprintf(fd, "buffer = ltt_reserve_slot(trace, relayfs_buf,\n");
	print_tabs(3, fd);
	fprintf(fd, "to_base + to + len, &slot_size, &tsc,\n");
	print_tabs(3, fd);
	fprintf(fd, "&before_hdr_pad, &after_hdr_pad);\n");
	/* If error, return */
	print_tabs(2, fd);
	fprintf(fd, "if(!buffer) return;\n\n");
	
	/* write data : assume stack alignment is the same as struct alignment. */

	for(unsigned int i=0;i<event->fields.position;i++){
		field_t *field = (field_t*)(event->fields.array[i]);
		type_descriptor_t *type = field->type;

		/* Set from */
		print_tabs(2, fd);
		switch(type->type) {
			case SEQUENCE:
			case UNION:
			case ARRAY:
			case STRUCT:
			case STRING:
				fprintf(fd, "from = %s;\n", field->name);
				break;
			default:
				fprintf(fd, "from = &%s;\n", field->name);
				break;
		}


		if(print_type_write(type,
				fd, 2, basename, field->name)) return 1;
		fprintf(fd, "\n");
		
		/* Don't forget to flush pending memcpy */
		print_tabs(2, fd);
		fprintf(fd, "/* Flush pending memcpy */\n");
		print_tabs(2, fd);
		fprintf(fd, "if(len != 0) {\n");
		print_tabs(3, fd);
		fprintf(fd, "memcpy(buffer+to_base+to, from, len);\n");
		print_tabs(3, fd);
		fprintf(fd, "to += len;\n");
		//print_tabs(3, fd);
		//fprintf(fd, "from += len;\n");
		print_tabs(3, fd);
		fprintf(fd, "len = 0;\n");
		print_tabs(2, fd);
		fprintf(fd, "}\n");
		fprintf(fd, "\n");
	}

	
	/* commit */
	print_tabs(2, fd);
	fprintf(fd, "ltt_commit_slot(relayfs_buf, buffer, slot_size);\n\n");
	
	print_tabs(1, fd);
	fprintf(fd, "}\n\n");

	/* Release locks */
	print_tabs(1, fd);
	fprintf(fd, "ltt_nesting[smp_processor_id()]--;\n");
	print_tabs(1, fd);
	fprintf(fd, "preempt_enable_no_resched();\n");

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
		/* Print also the align function */
		if((print_type_alignment_fct(types->array[i], fd,
						0, "", ""))) return 1;
		/* Print also the write function */
		if((print_type_write_fct(types->array[i], fd,
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
			if(t->type_name == NULL) {
				if((print_type_declaration(t, fd, 0, basename, f->name))) return 1;
				/* Print also the align function */
				if((print_type_alignment_fct(t, fd, 0, basename, f->name))) return 1;
				/* Print also the write function */
				if((print_type_write_fct(t, fd, 0, basename, f->name))) return 1;
			}
		}

		fprintf(fd, "\n");

		fprintf(fd, "/* Event %s logging function */\n",
				event->name);

		if(print_event_logging_function(basename, fac, event, fd)) return 1;

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
		
			generateChecksum(fac->name, &fac->checksum, &fac->events);
			
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


