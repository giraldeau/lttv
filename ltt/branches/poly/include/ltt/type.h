#ifndef TYPE_H
#define TYPE_H


/* Different types allowed */

typedef enum _LttTypeEnum 
{ LTT_INT, LTT_UINT, LTT_FLOAT, LTT_STRING, LTT_ENUM, LTT_ARRAY, 
  LTT_SEQUENCE, LTT_STRUCT, LTT_UNION
} LttTypeEnum;

#include <ltt/ltt.h>


/* All event types, data types and fields belong to their trace and 
   are released at the same time. */

/* Obtain the name, description, facility, facility relative id, global id, 
   type and root field for an eventtype */

char *ltt_eventtype_name(LttEventType *et);

char *ltt_eventtype_description(LttEventType *et);

LttFacility *ltt_eventtype_facility(LttEventType *et);

unsigned *ltt_eventtype_relative_id(LttEventType *et);

unsigned *ltt_eventtype_id(LttEventType *et);

LttType *ltt_eventtype_type(LttEventType *et);

LttField *ltt_eventtype_field(LttEventType *et);


/* obtain the type name and size. The size is the number of bytes for
   primitive types (INT, UINT, FLOAT, ENUM), or the size for the unsigned
   integer length count for sequences. */
 
char *ltt_type_name(LttType *t);

LttTypeEnum ltt_type_class(LttType *t);

unsigned ltt_type_size(LttTrace *trace, LttType *t); 


/* The type of nested elements for arrays and sequences. */

LttType *ltt_type_element_type(LttType *t);


/* The number of elements for arrays. */

unsigned ltt_type_element_number(LttType *t);


/* The number of data members for structures and unions. */

unsigned ltt_type_member_number(LttType *t);


/* The type of a data member in a structure. */

LttType *ltt_type_member_type(LttType *t, unsigned i, char ** name);


/* For enumerations, obtain the symbolic string associated with a value
   (0 to n - 1 for an enumeration of n elements). */

char *ltt_enum_string_get(LttType *t, unsigned i);


/* The fields form a tree representing a depth first search of the 
   corresponding event type directed acyclic graph. Fields for arrays and
   sequences simply point to one nested field representing the currently
   selected element among all the (identically typed) elements. For structures,
   a nested field exists for each data member. Each field stores the
   platform/trace specific offset values (for efficient access) and
   points back to the corresponding LttType for the rest. */

LttField *ltt_field_element(LttField *f);

LttField *ltt_field_member(LttField *f, unsigned i);

LttType *ltt_field_type(LttField *f);

#endif // TYPE_H
