#include <lttv/attribute.h>

inline lttv_string_id lttv_string_id_from_string(const char *s) {
  return g_quark_from_string(s);
}


inline void lttv_string_id_release(lttv_string_id i) {}


inline const char *lttv_string_id_to_string(lttv_string_id i) {
  return g_quark_to_string(i);
}


inline lttv_key *lttv_key_new() {
  return g_array_new(FALSE, FALSE, sizeof(lttv_string_id));
}

/* Changed this function to destroy the element also, caused memory leak? */
/* Mathieu Desnoyers */
inline void lttv_key_destroy(lttv_key *k) {
  g_array_free(k, TRUE);
}


#define _lttv_key_index(k,i) g_array_index(k, lttv_string_id, i)


inline void lttv_key_append(lttv_key *k, lttv_string_id i) {
  g_array_append_val(k,i);
}


inline unsigned int lttv_key_component_number(lttv_key *k) {
  return k->len;
}


lttv_key *lttv_key_copy(lttv_key *k) {
  lttv_key *nk;
  int i;

  nk = lttv_key_new();
  for(i = 0 ; i < k->len ; i++) lttv_key_append(nk,lttv_key_index(k,i));
  return nk;
}

/* It is also possible to create a key directly from a pathname,
   key components separated by /, (e.g., "/hooks/options/before"). */

lttv_key *lttv_key_new_pathname(const char *p) {
  char **v, **cursor;
  lttv_key *k;

  v = cursor = g_strsplit(p, "/", -1);
  k = lttv_key_new();

  while(*cursor != NULL) {
    lttv_key_append(k, lttv_string_id_from_string(*cursor));
    cursor++;
  }
  g_strfreev(v);
  return k;
}

static guint lttv_key_hash(gconstpointer key) {
  lttv_key * k = (lttv_key *)key;
  guint h = 0;
  int i;
  for(i = 0 ; i < k->len ; i++) h = h ^ lttv_key_index(k,i);
  return h;
}

static gboolean lttv_key_equal(gconstpointer key1,gconstpointer key2) {
  lttv_key * k1 = (lttv_key *)key1;
  lttv_key * k2 = (lttv_key *)key2;  
  int i;

  if(k1->len != k2->len) return FALSE;
  for(i = 0 ; i < k1->len ; i++) 
    if(lttv_key_index(k1,i) != lttv_key_index(k2,i)) return FALSE;
  return TRUE;
}


static void lttv_key_free(gpointer data)
{
  lttv_key_destroy((lttv_key *)data);
}


static void lttv_attribute_value_free(gpointer data) 
{
  g_free(data);
}


lttv_attributes *lttv_attributes_new() {
  lttv_attributes *a;

  a = g_new(lttv_attributes, 1);
  a->ints = g_hash_table_new_full(lttv_key_hash, lttv_key_equal,
      lttv_key_free, lttv_attribute_value_free);
  a->times = g_hash_table_new_full(lttv_key_hash, lttv_key_equal,
      lttv_key_free, lttv_attribute_value_free);
  a->doubles = g_hash_table_new_full(lttv_key_hash, lttv_key_equal,
      lttv_key_free, lttv_attribute_value_free);
  a->pointers = g_hash_table_new(lttv_key_hash, lttv_key_equal);

  return a;
}


/* Free the hash table containing the stats and all the contained keys/vals */

static void lttv_attribute_key_free(gpointer k, gpointer v, gpointer data) {
  lttv_key_free(k);
}


void lttv_attributes_destroy(lttv_attributes *a) {
  g_hash_table_destroy(a->ints);
  g_hash_table_destroy(a->times);
  g_hash_table_destroy(a->doubles);

  g_hash_table_foreach(a->pointers, lttv_attribute_key_free, NULL);
  g_hash_table_destroy(a->pointers);
  g_free(a);
}

unsigned int lttv_attributes_number(lttv_attributes *a) {
  return g_hash_table_size(a->ints) + g_hash_table_size(a->times) +
      g_hash_table_size(a->doubles) + g_hash_table_size(a->pointers);
}


/* If it is a new entry, insert it in the hash table, and set it to 0 */

int *lttv_attributes_get_integer(lttv_attributes *a, lttv_key *k) 
{
  gpointer found;

  found = g_hash_table_lookup(a->ints, k);
  if(found == NULL) {
    found = g_new(gint, 1);
    *(gint *)found = 0;
    g_hash_table_insert(a->ints, lttv_key_copy(k), found);
  }
  return found;
}


lttv_time *lttv_attributes_get_time(lttv_attributes *a, lttv_key *k) 
{
  gpointer found;
  
  found = g_hash_table_lookup(a->times, k);
  if(found == NULL) {
    found = g_new0(lttv_time, 1);
    /*    *(lttv_time *)found = ZeroTime; */
    g_hash_table_insert(a->times, lttv_key_copy(k), found);
  }
  return found;
}

double *lttv_attributes_get_double(lttv_attributes *a, lttv_key *k) 
{
  gpointer found;

  found = g_hash_table_lookup(a->doubles,k);
  if(found == NULL) {
    found = g_new(double,1);
    *(double *)found = 0;
    g_hash_table_insert(a->doubles, lttv_key_copy(k),found);
  }
  return found;
}

void *lttv_attributes_get_pointer_pathname(lttv_attributes *a, char *pn) 
{
  lttv_key *key;
  void *p;

  key = lttv_key_new_pathname(pn);
  p = lttv_attributes_get_pointer(a, key);
  lttv_key_destroy(key);

  return p;
}

void *lttv_attributes_get_pointer(lttv_attributes *a, lttv_key *k) 
{
  return g_hash_table_lookup(a->pointers,k);
}

void lttv_attributes_set_pointer_pathname(lttv_attributes *a,char *pn,void *p)
{
  lttv_key *key = lttv_key_new_pathname(pn);
  
  lttv_attributes_set_pointer(a, key, p);
  lttv_key_destroy(key);
}

void lttv_attributes_set_pointer(lttv_attributes *a, lttv_key *k, void *p) {
  lttv_key * oldk;
  void *oldv;

  if(g_hash_table_lookup_extended(a->pointers, k, (gpointer)oldk, &oldv)) {
    if(p == NULL) {
      g_hash_table_remove(a->pointers,k);
    } 
    else {
      g_hash_table_insert(a->pointers,oldk,p);
    }
  } 
  else {
    if(p == NULL) return;
    g_hash_table_insert(a->pointers,lttv_key_copy(k),p);
  }
}


#ifdef EXT_ATTRIBS
/* Sometimes the attributes must be accessed in bulk, sorted in different
   ways. For this purpose they may be converted to arrays and sorted
   multiple times. The keys used in the array belong to the lttv_attributes
   object from which the array was obtained and are freed when it is
   destroyed. Each element in the array is an lttv_attribute, a structure
   containing the key, the value type, and a union containing a value of
   that type. Multiple attributes with equal keys may be possible in some
   implementations if their type differs. */


typedef struct _lttv_attribute_fill_position  {
  unsigned i;
  lttv_attribute_type t;
  lttv_attribute *a;
} lttv_attribute_fill_position;


static void lttv_attribute_fill(void *key, void *value, void *user_data) {
  lttv_attribute_fill_position * p = (lttv_attribute_fill_position *)user_data;
  lttv_attribute *a = p->a + p->i;

  a->key = (lttv_key *)key;
  a->t = p->t;
  switch(p->t) {
    case LTTV_INTEGER:
      a->v.i = *((int *)value);
    case LTTV_TIME:
      a->v.t = *((lttv_time *)value);
    case LTTV_DOUBLE:
      a->v.d = *((double *)value);
    case LTTV_POINTER:
      a->v.p = value;
  }
  p->i++;
}


lttv_attribute *lttv_attributes_array_get(lttv_attributes *a) {
  unsigned size;
  lttv_attribute *v;
  lttv_attribute_fill_position p;

  size = lttv_attributes_number(a);
  v = g_new(lttv_attribute,size);

  p.a = v;
  p.i = 0;
  p.t = LTTV_INTEGER;
  g_hash_table_foreach(a->ints, lttv_attribute_fill, &p);
  p.t = LTTV_TIME;
  g_hash_table_foreach(a->times, lttv_attribute_fill, &p);
  p.t = LTTV_DOUBLE;
  g_hash_table_foreach(a->doubles, lttv_attribute_fill, &p);
  p.t = LTTV_POINTER;
  g_hash_table_foreach(a->pointers, lttv_attribute_fill, &p);
  return v;
}


lttv_attribute *lttv_attribute_array_destroy(lttv_attribute *a) {
  g_free(a);
}


void lttv_attribute_array_sort(lttv_attribute *a,
    unsigned size, lttv_key_compare f,
    void *compare_data)
{
  
  g_qsort_with_data(a, size, sizeof(lttv_attribute), f, 
      compare_data);
}


int lttv_key_compare_priority(lttv_key *a, lttv_key *b, void *compare_data)
{
  int i, res;
  int *priority = (int *)compare_data;

  g_assert(a->len == b->len);

  for(i = 0 ; i < a->len ; i++) 
  {
    res = strcmp(lttv_string_id_to_string(lttv_key_index(a,priority[i])),
        lttv_string_id_to_string(lttv_key_index(a,priority[i])));
    if(res != 0) return res;
  }
  return 0;
}

typedef struct _select_data {
  lttv_attributes *a;
  lttv_key *k;
  void *user_data;
  lttv_key_select select;
} select_data;

static void select_integer(void *key, void *value, void *user_data);
static void select_double(void *key, void *value, void *user_data);
static void select_time(void *key, void *value, void *user_data);
static void select_pointer(void *key, void *value, void *user_data);

lttv_attributes *lttv_attributes_select(lttv_attributes *a, lttv_key_select f, 
    void *user_data)
{
  select_data *d;

  d = g_new(select_data, 1);
  d->a = lttv_attributes_new();
  d->k = lttv_key_new();
  d->user_data = user_data;
  d->select = f;

  g_hash_table_foreach(a->ints,select_integer, d);
  g_hash_table_foreach(a->doubles,select_double, d);
  g_hash_table_foreach(a->times,select_time, d);
  g_hash_table_foreach(a->pointers,select_pointer, d);
}

int lttv_key_select_spec(lttv_key *in, lttv_key *out, void *user_data)
{
  lttv_key_select_spec_data *d = (lttv_key_select_spec_data *)user_data;
  int i;

  /* not defined yet */
  /* lttv_key_set_size(out, 0); */

  for(i = 0 ; i < d->length ; i++) {
    switch(d->spec[i]) {
    case LTTV_KEEP:
      break;

    case LTTV_KEEP_EQUAL:
      break;

    case LTTV_KEEP_SMALLER:
      break;

    case LTTV_KEEP_GREATER:
      break;

    case LTTV_IGNORE:
      break;

    }
  }

  return 1;
}

static void select_integer(void *key, void *value, void *user_data)
{
  lttv_key *k = (lttv_key *)key;
  int *pi = (int *)value;
  select_data *d = (select_data *)user_data;

  if(d->select(k,d->k,d->user_data)) {
    *(lttv_attributes_get_integer(d->a,d->k)) += *pi;
  }
}

static void select_double(void *key, void *value, void *user_data)
{
  lttv_key *k = (lttv_key *)key;
  double *pf = (double *)value;
  select_data *d = (select_data *)user_data;

  if(d->select(k,d->k,d->user_data)) {
    *(lttv_attributes_get_double(d->a,d->k)) += *pf;
  }
}

static void select_time(void *key, void *value, void *user_data)
{
  lttv_key *k = (lttv_key *)key;
  lttv_time *ptSum, *pt = (lttv_time *)value;
  select_data *d = (select_data *)user_data;

  if(d->select(k,d->k,d->user_data)) {
    ptSum = lttv_attributes_get_time(d->a,d->k);
    ptSum->tv_sec += pt->tv_sec;
    ptSum->tv_nsec += pt->tv_nsec;
  }
}

static void select_pointer(void *key, void *value, void *user_data)
{
  lttv_key *k = (lttv_key *)key;
  select_data *d = (select_data *)user_data;

  if(d->select(k,d->k,d->user_data)) {
    lttv_attributes_set_pointer(d->a,d->k,value);
  }
}





#endif // EXT_ATTRIBS
