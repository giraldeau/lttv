#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H


#include <glib.h>
#include <time.h>

/* Attributes are used to store any value identified by a key. They are
   typically used to store state information or accumulated statistics for
   some object. Each value is accessed through a multi-component key, which 
   resembles hierarchical pathnames in filesystems.

   The attributes may store integers, doubles or time values, in which case
   the values are created upon first access of a key and with a default 
   value of 0. Pointer values are also available with a default value of NULL
   and must thus be set explicitely to user managed (statically or dynamically
   allocated) memory.  */


typedef guint32 lttv_string_id;

typedef GArray _lttv_key;

typedef _lttv_key lttv_key;

typedef struct timespec lttv_time;


typedef struct _lttv_attributes {
  GHashTable *ints;
  GHashTable *times;
  GHashTable *doubles;
  GHashTable *pointers;
} lttv_attributes;


/* A unique integer identifier represents each different string
   used as a key component. A single copy of each different string is
   stored but its usage count is incremented each time the corresponding id
   is returned by lttv_string_id_from_string. The usage count is decremented
   each time an id is released. */

lttv_string_id lttv_string_id_from_string(const char *s);

void lttv_string_id_release(lttv_string_id i);

const char *lttv_string_id_to_string(lttv_string_id i);


/* Keys are created and subsequently filled with key components */

lttv_key *lttv_key_new();

void lttv_key_destroy(lttv_key *k);

/* macro to access/replace a the i th component of key k */

#define lttv_key_index(k,i) _lttv_key_index(k,i)


/* Append a new component */

void lttv_key_append(lttv_key *k, lttv_string_id i);


/* Number of components in a key */

unsigned int lttv_key_number(lttv_key *k);


/* It is also possible to create a key directly from a pathname,
   key components separated by /, (e.g., "/hooks/options/before"). */

lttv_key *lttv_key_new_pathname(const char *pathname);


/* Create a new set of attributes */

lttv_attributes *lttv_attributes_new();


/* Destroy the set of attributes including all the memory allocated
   internally for it (copies of keys, and integer, double and time 
   values...). */

void lttv_attributes_destroy(lttv_attributes *a);


/* Total number of attributes in a lttv_attributes. */

unsigned int lttv_attributes_number(lttv_attributes *a);


/* Obtain a pointer to the value of the corresponding type associated with
   the specified key. New values are created on demand with 0 as initial
   value. These values are freed when the attributes set is destroyed. */

int *lttv_attributes_get_integer(lttv_attributes *a, lttv_key *k);

lttv_time *lttv_attributes_get_time(lttv_attributes *a, lttv_key *k);

double *lttv_attributes_get_double(lttv_attributes *a, lttv_key *k);


/* Set or get the pointer value associated with the specified key. 
   NULL is returned if no pointer was set for the key. */

void *lttv_attributes_get_pointer(lttv_attributes *a, lttv_key *k);

void lttv_attributes_set_pointer(lttv_attributes *a, lttv_key *k, void *p);

void *lttv_attributes_get_pointer_pathname(lttv_attributes *a, char *pn);

void lttv_attributes_set_pointer_pathname(lttv_attributes *a,char *pn,void *p);


/* It is often useful to copy over some elements from the source attributes 
   table to the destination table. While doing so, constraints on each key 
   component may be used to select the elements of interest. Finally, some 
   numerical elements may need to be summed, for example summing the number 
   of page faults over all processes. A flexible function to copy attributes 
   may be used for all these operations. 

   If the key of the element copied already exists in the destination 
   attributes, numerical values (integer, double or time) are summed and 
   pointers are replaced.

   The lttv_key_select_data structure specifies for each key component the
   test applied to decide to copy or not the corresponding element. 
   It contains the relation to apply to each key component, the rel vector, 
   and the comparison key, both of size length. To decide if an element
   should be copied, each component of its key is compared with the
   comparison key, and the relation specified for each component must 
   be verified. The relation ANY is always verified and the comparison key
   component is not used. The relation NONE is only verified if the key
   examined contains fewer components than the position examined. The EQ, NE,
   LT, LE, GT, GE relations are verified if the key is long enough and the
   component satisfies the relation with respect to the comparison key.
   Finally, the CUT relation is satisfied if the key is long enough, but the
   element is copied with that component removed from its key. All the keys
   which only differ by that component become identical after being shortened
   and their numerical values are thus summed when copied. */

typedef enum _lttv_key_select_relation
{ LTTV_KEY_ANY,   /* Any value is good */
  LTTV_KEY_NONE,  /* No value is good, i.e. the key must be shorter */
  LTTV_KEY_EQ,    /* key[n] is equal to match[n] */
  LTTV_KEY_NE,    /* key[n] is not equal to match[n] */
  LTTV_KEY_LT,    /* key[n] is lower than match[n] */
  LTTV_KEY_LE,    /* key[n] is lower or equal than match[n] */
  LTTV_KEY_GT,    /* key[n] is greater than match[n] */
  LTTV_KEY_GE,    /* key[n] is greater or equal than match[n] */
  LTTV_KEY_CUT    /* cut key[n], shortening the key for the copy */
} lttv_key_select_relation;

typedef struct _lttv_key_select_data
{
  unsigned length;
  lttv_key_select_relation *rel;
  lttv_key *comparison;
} lttv_key_select_data;

void lttv_attributes_copy(lttv_attributes *src, lttv_attributes *dest, 
    lttv_key_select_data d);


/* Sometimes the attributes must be accessed in bulk, sorted in different
   ways. For this purpose they may be converted to arrays and sorted
   multiple times. The keys used in the array belong to the lttv_attributes
   object from which the array was obtained and are freed when it is
   destroyed. Each element in the array is an lttv_attribute, a structure
   containing the key, the value type, and a union containing a value of
   that type. Multiple attributes with equal keys may be possible in some
   implementations if their type differs. */


typedef enum _lttv_attribute_type 
{ LTTV_INTEGER, LTTV_TIME, LTTV_DOUBLE, LTTV_POINTER } 
lttv_attribute_type;

typedef union _lttv_value_any
{ int i; 
  lttv_time t; 
  double d;
  void *p;
} lttv_value_any;

typedef struct _lttv_attribute {
  lttv_key *key;
  lttv_attribute_type t;
  lttv_value_any v;
} lttv_attribute;


/* Obtain all the attributes in an array */

lttv_attribute *lttv_attributes_array_get(lttv_attributes *a);

lttv_attribute *lttv_attribute_array_destroy(lttv_attribute *a);


/* The sorting order is determined by the supplied comparison function.
   The comparison function must return a negative value if a < b, 
   0 if a = b, and a positive if a > b. */

typedef int (*lttv_key_compare)(lttv_key *a, lttv_key *b, void *user_data);

void lttv_attribute_array_sort(lttv_attribute *a, unsigned size, 
    lttv_key_compare f, void *user_data);


/* Sort in lexicographic order using the specified key components as primary,
   secondary... keys. A vector containing the key components positions is
   specified. */
 
int lttv_attribute_array_sort_lexicographic(lttv_attribute *a, unsigned size,
    unsigned *positions, unsigned nb_positions);

#endif // ATTRIBUTE_H
