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

void preset_field_type_size(event_t *event_type,
    off_t offset_root, off_t offset_parent,
    enum field_status *fixed_root, enum field_status *fixed_parent,
    field_t *field);

/* Code printing */

void print_tabs(unsigned int tabs, FILE *fd)
{
	for(unsigned int i = 0; i<tabs;i++)
		fprintf(fd, "\t");
}

/* Type size checking */
/* Uses #error in the generated code to signal error and stop the compiler */
int print_check(int fd);


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
			fprintf(fd, "#define LTTNG_ARRAY_SIZE_%s %llu\n", basename,
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
			fprintf(fd, "unsigned int len;\n");
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





/*****************************************************************************
 *Function name
 *    set_named_type_offsets : set the precomputable offset of the named type
 *Input params 
 *    type : the type
 ****************************************************************************/
void set_named_type_offsets(type_descriptor_t *type)
{
  enum field_status	current_child_status = FIELD_FIXED_GENEVENT;
	off_t current_offset = 0;

	preset_type_size(
			current_offset,
			&current_child_status,
			type);
	if(current_child_status == FIELD_FIXED_GENEVENT) {
		current_offset += type->size;
	} else {
		current_offset = 0;
	}
}


/*****************************************************************************
 *Function name
 *    set_event_fields_offsets : set the precomputable offset of the fields
 *Input params 
 *    event           : the event
 ****************************************************************************/
void set_event_fields_offsets(event_t *event)
{
  enum field_status current_child_status = FIELD_FIXED;
	off_t current_offset = 0;

	for(unsigned int i = 0; i < event->type->fields.position; i++) {
		/* For each field, set the field offset. */
		field_t *child_field = (field_t*)event->type->fields.array[i];
		type_descriptor_t *t = f->type;
		/* Skip named types */
		if(t->type_name != NULL) continue;

    preset_type_size(
				current_offset,
        &current_child_status,
        t);
		if(current_child_status == FIELD_FIXED_GENEVENT) {
			current_offset += type->size;
		} else {
			current_offset = 0;
		}
	}
}



/*****************************************************************************
 *Function name
 *    print_type_size : print the fixed sizes of the field type
 *    taken from LTTV.
 *
 *    use offset_parent as offset to calculate alignment.
 *Input params 
 *    offset_parent   : offset from the parent
 *    fixed_parent    : Do we know a fixed offset to the parent ?
 *    type 	          : type
 ****************************************************************************/
void print_type_size(
    off_t offset_parent,
    enum field_status *fixed_parent,
    type_descriptor_t *type,
		FILE *fd,
		char *size_name)
{
  enum field_status local_fixed_parent;
  guint i;
  
  g_assert(type->fixed_size == FIELD_UNKNOWN);

  size_t current_offset;
  enum field_status current_child_status, final_child_status;
  size_t max_size;

  switch(type->type) {
			/* type sizes known by genevent/compiler */
		case INT_FIXED:
		case UINT_FIXED:
    case FLOAT:
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
			/* Align */
			fprintf(fd, "%s += ltt_align(%s, %s)", size_name);
			/* Data */
      type->fixed_size = FIELD_FIXED_GENEVENT;
      break;
			/* host type sizes unknown by genevent, but known by the compiler */
    case INT:
    case UINT:
    case ENUM:
			/* An enum is either : char or int. In gcc, always int. Hope
			 * it's always like this. */
			type->fixed_size = FIELD_FIXED_COMPILER;
			type->compiler_size = COMPILER_INT;
			break;
    case LONG:
    case ULONG:
			type->fixed_size = FIELD_FIXED_COMPILER;
			type->compiler_size = COMPILER_LONG;
			break;
    case POINTER:
			type->fixed_size = FIELD_FIXED_COMPILER;
			type->compiler_size = COMPILER_POINTER;
			break;
    case SIZE_T:
    case SSIZE_T:
    case OFF_T:
			type->fixed_size = FIELD_FIXED_COMPILER;
			type->compiler_size = COMPILER_SIZE_T;
			break;
			/* compound types :
			 * if all children has fixed size, then the parent size can be
			 * known directly and the copy can be done efficiently.
			 * if only part of the children has fixed size, then the contiguous
			 * elements will be copied in one bulk, but the variable size elements
			 * will be copied separately. This is necessary because those variable
			 * size elements are referenced by pointer in C.
			 */
    case SEQUENCE:
      current_offset = 0;
      local_fixed_parent = FIELD_FIXED_GENEVENT;
      preset_type_size(
        0,
        &local_fixed_parent,
        ((field_t*)type->fields.array[0])->type);
      preset_field_type_size(
        0,
        &local_fixed_parent,
        ((field_t*)type->fields.array[1])->type);
      type->fixed_size = FIELD_VARIABLE;
      *fixed_parent = FIELD_VARIABLE;
      break;
    case LTT_STRING:
      field->fixed_size = FIELD_VARIABLE;
      *fixed_parent = FIELD_VARIABLE;
      break;
    case LTT_ARRAY:
      local_fixed_parent = FIELD_FIXED_GENEVENT;
      preset_type_size(
        0,
        &local_fixed_parent,
        ((field_t*)type->fields.array[0])->type);
      type->fixed_size = local_fixed_parent;
      if(type->fixed_size == FIELD_FIXED_GENEVENT) {
        type->size =
					type->element_number * ((field_t*)type->fields.array[0])->type->size;
      } else if(type->fixed_size == FIELD_FIXED_COMPILER) {
        type->size =
					type->element_number;
      } else {
        type->size = 0;
        *fixed_parent = FIELD_VARIABLE;
      }
      break;
    case LTT_STRUCT:
      current_offset = 0;
      current_child_status = FIELD_FIXED_GENEVENT;
      for(i=0;i<type->element_number;i++) {
        preset_field_type_size(
          current_offset, 
          &current_child_status,
          ((field_t*)type->fields.array[i])->type);
        if(current_child_status == FIELD_FIXED_GENEVENT) {
          current_offset += field->child[i]->field_size;
        } else {
          current_offset = 0;
        }
      }
      if(current_child_status != FIELD_FIXED_GENEVENT) {
        *fixed_parent = current_child_status;
        type->size = 0;
        type->fixed_size = current_child_status;
      } else {
        type->size = current_offset;
        type->fixed_size = FIELD_FIXED_GENEVENT;
      }
      break;
    case LTT_UNION:
      current_offset = 0;
      max_size = 0;
      final_child_status = FIELD_FIXED_GENEVENT;
      for(i=0;i<type->element_number;i++) {
        enum field_status current_child_status = FIELD_FIXED;
        preset_field_type_size(
          current_offset, 
          &current_child_status,
          ((field_t*)type->fields.array[i])->type);
        if(current_child_status != FIELD_FIXED_GENEVENT)
          final_child_status = current_child_status;
        else
          max_size =
						max(max_size, ((field_t*)type->fields.array[i])->type->size);
      }
      if(final_child_status != FIELD_FIXED_GENEVENT AND COMPILER) {
				g_error("LTTV does not support variable size fields in unions.");
				/* This will stop the application. */
        *fixed_parent = final_child_status;
        type->size = 0;
        type->fixed_size = current_child_status;
      } else {
        type->size = max_size;
        type->fixed_size = FIELD_FIXED_GENEVENT;
      }
      break;
  }

}


size_t get_field_type_size(LttTracefile *tf, LttEventType *event_type,
    off_t offset_root, off_t offset_parent,
    LttField *field, void *data)
{
  size_t size = 0;
  guint i;
  LttType *type;
  
  g_assert(field->fixed_root != FIELD_UNKNOWN);
  g_assert(field->fixed_parent != FIELD_UNKNOWN);
  g_assert(field->fixed_size != FIELD_UNKNOWN);

  field->offset_root = offset_root;
  field->offset_parent = offset_parent;
  
  type = field->field_type;

  switch(type->type_class) {
    case LTT_INT:
    case LTT_UINT:
    case LTT_FLOAT:
    case LTT_ENUM:
    case LTT_POINTER:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
      g_assert(field->fixed_size == FIELD_FIXED);
      size = field->field_size;
      break;
    case LTT_SEQUENCE:
      {
        gint seqnum = ltt_get_uint(LTT_GET_BO(tf),
                        field->sequ_number_size,
                        data + offset_root);

        if(field->child[0]->fixed_size == FIELD_FIXED) {
          size = field->sequ_number_size + 
            (seqnum * get_field_type_size(tf, event_type,
                                          offset_root, offset_parent,
                                          field->child[0], data));
        } else {
          size += field->sequ_number_size;
          for(i=0;i<seqnum;i++) {
            size_t child_size;
            child_size = get_field_type_size(tf, event_type,
                                    offset_root, offset_parent,
                                    field->child[0], data);
            offset_root += child_size;
            offset_parent += child_size;
            size += child_size;
          }
        }
        field->field_size = size;
      }
      break;
    case LTT_STRING:
      size = strlen((char*)(data+offset_root)) + 1;// length + \0
      field->field_size = size;
      break;
    case LTT_ARRAY:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        for(i=0;i<field->field_type->element_number;i++) {
          size_t child_size;
          child_size = get_field_type_size(tf, event_type,
                                  offset_root, offset_parent,
                                  field->child[0], data);
          offset_root += child_size;
          offset_parent += child_size;
          size += child_size;
        }
        field->field_size = size;
      }
      break;
    case LTT_STRUCT:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        size_t current_root_offset = offset_root;
        size_t current_offset = 0;
        size_t child_size = 0;
        for(i=0;i<type->element_number;i++) {
          child_size = get_field_type_size(tf,
                     event_type, current_root_offset, current_offset, 
                     field->child[i], data);
          current_offset += child_size;
          current_root_offset += child_size;
          
        }
        size = current_offset;
        field->field_size = size;
      }
      break;
    case LTT_UNION:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        size_t current_root_offset = field->offset_root;
        size_t current_offset = 0;
        for(i=0;i<type->element_number;i++) {
          size = get_field_type_size(tf, event_type,
                 current_root_offset, current_offset, 
                 field->child[i], data);
          size = max(size, field->child[i]->field_size);
        }
        field->field_size = size;
      }
      break;
  }

  return size;
}





/* Print the code that calculates the length of an field */
int print_field_len(type_descriptor_t * td, FILE *fd, unsigned int tabs,
		char *nest_type_name, char *nest_field_name, char *field_name, 
		char *output_var, char *member_var)
{
	/* Type name : basename */
	char basename[PATH_MAX];
	unsigned int basename_len = 0;

	strcpy(basename, nest_type_name);
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
	
	/* Field name : basefieldname */
	char basefieldname[PATH_MAX];
	unsigned int basefieldname_len = 0;

	strcpy(basefieldname, nest_field_name);
	basefieldname_len = strlen(basefieldname);
	
	/* there must be a field name */
	strncat(basefieldname, field_name, PATH_MAX - basefieldname_len);
	basefieldname_len = strlen(basefieldname);

	print_tabs(tabs, fd);

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
			fprintf(fd, "/* Size of %s */", field_name);
			print_tabs(tabs, fd);
			fprintf(fd, "%s = sizeof(", member_var);
			if(print_type(td, fd, tabs, basename, "")) return 1;
			fprintf(fd, ");\n");
			fprintf(fd, "%s += ltt_align(%s, %s);\n", output_var, member_var);
			print_tabs(tabs, fd);
			fprintf(fd, "%s += %s;\n", output_var, member_var);
			break;
		case STRING:
			/* strings are made of bytes : no alignment. */
			fprintf(fd, "/* Size of %s */", basefieldname);
			print_tabs(tabs, fd);
			fprintf(fd, "%s = strlen(%s);", member_var, basefieldname);
			break;
		case ARRAY:
			fprintf(fd, "/* Size of %s */", basefieldname);
			print_tabs(tabs, fd);

			strncat(basefieldname, ".", PATH_MAX - basefieldname_len);
			basefieldname_len = strlen(basefieldname);

			if(print_field_len(((field_t*)td->fields.array[0])->type,
						fd, tabs,
						basename, basefieldname,
						output_var, member_var)) return 1;

			fprintf(fd, "%s = strlen(%s);", member_var, basefieldname);

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




/* Print the logging function of an event. This is the core of genevent */
int print_event_logging_function(char *basename, event_t *event, FILE *fd)
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
	fprintf(fd, "size_t member_length;");
	fprintf(fd, "size_t event_length = 0;");
	
	/* Calculate event variable len + event data alignment offset.
	 * Assume that the padding for alignment starts at a void*
	 * address.
	 * This excludes the header size and alignment. */


	for(unsigned int j = 0; j < event->fields.position; j++) {
		/* For each field, calculate the field size. */
		field_t *f = (field_t*)event->fields.array[j];
		type_descriptor_t *t = f->type;
		if(print_field_len(t, fd, 1, basename, "", 
					f->name,
					"event_length",
					"member_length")) return 1;
		if(j < event->fields.position-1) {
			fprintf(fd, ",");
			fprintf(fd, "\n");
		}
	}

	/* Take locks : make sure the trace does not vanish while we write on
	 * it. A simple preemption disabling is enough (using rcu traces). */
	
	/* For each trace */
	
	/* Relay reserve */
	/* If error, increment counter and return */
	
	/* write data */
		
	/* commit */
	
	/* Release locks */

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


