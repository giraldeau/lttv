
#include <lttv/attribute.h>
#include <ltt/ltt.h>

typedef union _AttributeValue {
  int dv_int;
  unsigned dv_uint;
  long dv_long;
  unsigned long dv_ulong;
  float dv_float;
  double dv_double;
  LttvTime dv_time;
  gpointer dv_pointer;
  char *dv_string;
  GObject *dv_gobject;
} AttributeValue;


typedef struct _Attribute {
  LttvAttributeName name;
  LttvAttributeType type;
  AttributeValue value;
} Attribute;


LttvAttributeValue address_of_value(LttvAttributeType t, AttributeValue *v)
{
  LttvAttributeValue va;

  switch(t) {
  case LTTV_INT: va.v_int = &v->dv_int; break;
  case LTTV_UINT: va.v_uint = &v->dv_uint; break;
  case LTTV_LONG: va.v_long = &v->dv_long; break;
  case LTTV_ULONG: va.v_ulong = &v->dv_ulong; break;
  case LTTV_FLOAT: va.v_float = &v->dv_float; break;
  case LTTV_DOUBLE: va.v_double = &v->dv_double; break;
  case LTTV_TIME: va.v_time = &v->dv_time; break;
  case LTTV_POINTER: va.v_pointer = &v->dv_pointer; break;
  case LTTV_STRING: va.v_string = &v->dv_string; break;
  case LTTV_GOBJECT: va.v_gobject = &v->dv_gobject; break;
  }
  return va;
}


AttributeValue init_value(LttvAttributeType t)
{
  AttributeValue v;

  switch(t) {
  case LTTV_INT: v.dv_int = 0; break;
  case LTTV_UINT: v.dv_uint = 0; break;
  case LTTV_LONG: v.dv_long = 0; break;
  case LTTV_ULONG: v.dv_ulong = 0; break;
  case LTTV_FLOAT: v.dv_float = 0; break;
  case LTTV_DOUBLE: v.dv_double = 0; break;
  case LTTV_TIME: v.dv_time.tv_sec = 0; v.dv_time.tv_nsec = 0; break;
  case LTTV_POINTER: v.dv_pointer = NULL; break;
  case LTTV_STRING: v.dv_string = NULL; break;
  case LTTV_GOBJECT: v.dv_gobject = NULL; break;
  }
  return v;
}


unsigned int 
lttv_attribute_get_number(LttvAttribute *self)
{
  return self->attributes->len;
}


gboolean 
lttv_attribute_named(LttvAttribute *self, gboolean *homogeneous)
{
  *homogeneous = FALSE;
  return TRUE;
}


LttvAttributeType 
lttv_attribute_get(LttvAttribute *self, unsigned i, LttvAttributeName *name, 
    LttvAttributeValue *v)
{
  Attribute *a;

  a = &g_array_index(self->attributes, Attribute, i);
  *name = a->name;
  *v = address_of_value(a->type, &(a->value));
  return a->type;
}


LttvAttributeType 
lttv_attribute_get_by_name(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeValue *v)
{
  Attribute *a;

  unsigned i;

  gpointer p;

  p = g_hash_table_lookup(self->names, GUINT_TO_POINTER(name));
  if(p == NULL) return LTTV_NONE;

  i = GPOINTER_TO_UINT(p);
  i--;
  a = &g_array_index(self->attributes, Attribute, i);
  *v = address_of_value(a->type, &(a->value));
  return a->type;
}


LttvAttributeValue 
lttv_attribute_add(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t)
{
  unsigned i;

  Attribute a, *pa;

  i = (unsigned)g_hash_table_lookup(self->names, GUINT_TO_POINTER(name));
  if(i != 0) g_error("duplicate entry in attribute table");

  a.name = name;
  a.type = t;
  a.value = init_value(t);
  g_array_append_val(self->attributes, a);
  i = self->attributes->len - 1;
  pa = &g_array_index(self->attributes, Attribute, i);
  g_hash_table_insert(self->names, GUINT_TO_POINTER(name), 
      GUINT_TO_POINTER(i + 1));
  return address_of_value(t, &(pa->value));
}


/* Remove an attribute */

void 
lttv_attribute_remove(LttvAttribute *self, unsigned i)
{
  Attribute *a;

  a = &g_array_index(self->attributes, Attribute, i);

  /* Remove the array element and its entry in the name index */

  g_hash_table_remove(self->names, GUINT_TO_POINTER(a->name));
  g_array_remove_index_fast(self->attributes, i);

  /* The element used to replace the removed element has its index entry
     all wrong now. Reinsert it with its new position. */

  if(self->attributes->len != i){
    g_hash_table_remove(self->names, GUINT_TO_POINTER(a->name));
    g_hash_table_insert(self->names, GUINT_TO_POINTER(a->name), GUINT_TO_POINTER(i + 1));
  }
}

void 
lttv_attribute_remove_by_name(LttvAttribute *self, LttvAttributeName name)
{
  unsigned i;

  i = (unsigned)g_hash_table_lookup(self->names, GUINT_TO_POINTER(name));
  if(i == 0) g_error("remove by name non existent attribute");

  lttv_attribute_remove(self, i - 1);
}

/* Create an empty iattribute object and add it as an attribute under the
   specified name, or return an existing iattribute attribute. If an
   attribute of that name already exists but is not a GObject supporting the
   iattribute interface, return NULL. */

/*CHECK*/LttvAttribute* 
lttv_attribute_find_subdir(LttvAttribute *self, LttvAttributeName name)
{
  unsigned i;

  Attribute a;

  LttvAttribute *new;

  i = (unsigned)g_hash_table_lookup(self->names, GUINT_TO_POINTER(name));
  if(i != 0) {
    a = g_array_index(self->attributes, Attribute, i - 1);
    if(a.type == LTTV_GOBJECT && LTTV_IS_IATTRIBUTE(a.value.dv_gobject)) {
      return LTTV_ATTRIBUTE(a.value.dv_gobject);
    }
    else return NULL;    
  }
  new = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  *(lttv_attribute_add(self, name, LTTV_GOBJECT).v_gobject) = G_OBJECT(new);
  return (LttvAttribute *)new;
}

gboolean 
lttv_attribute_find(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  unsigned i;

  Attribute *a;

  i = (unsigned)g_hash_table_lookup(self->names, GUINT_TO_POINTER(name));
  if(i != 0) {
    a = &g_array_index(self->attributes, Attribute, i - 1);
    if(a->type != t) return FALSE;
    *v = address_of_value(t, &(a->value));
    return TRUE;
  }

  *v = lttv_attribute_add(self, name, t);
  return TRUE;
}


void lttv_attribute_recursive_free(LttvAttribute *self)
{
  int i, nb;

  Attribute *a;

  nb = self->attributes->len;

  for(i = 0 ; i < nb ; i++) {
    a = &g_array_index(self->attributes, Attribute, i);
    if(a->type == LTTV_GOBJECT && LTTV_IS_ATTRIBUTE(a->value.dv_gobject)) {
      lttv_attribute_recursive_free((LttvAttribute *)(a->value.dv_gobject));
    }
  }
  g_object_unref(self);
}


void lttv_attribute_recursive_add(LttvAttribute *dest, LttvAttribute *src)
{
  int i, nb;

  Attribute *a;

  LttvAttributeValue value;

  nb = src->attributes->len;

  for(i = 0 ; i < nb ; i++) {
    a = &g_array_index(src->attributes, Attribute, i);
    if(a->type == LTTV_GOBJECT && LTTV_IS_ATTRIBUTE(a->value.dv_gobject)) {
      lttv_attribute_recursive_add(
      /*CHECK*/(LttvAttribute *)lttv_attribute_find_subdir(dest, a->name),
          (LttvAttribute *)(a->value.dv_gobject));
    }
    else {
      g_assert(lttv_attribute_find(dest, a->name, a->type, &value));
      switch(a->type) {
	case LTTV_INT:
          *value.v_int += a->value.dv_int;
          break;
        case LTTV_UINT:
          *value.v_uint += a->value.dv_uint;
          break;
        case LTTV_LONG:
          *value.v_long += a->value.dv_long;
          break;
        case LTTV_ULONG:
          *value.v_ulong += a->value.dv_ulong;
          break;
        case LTTV_FLOAT:
          *value.v_float += a->value.dv_float;
          break;
        case LTTV_DOUBLE:
          *value.v_double += a->value.dv_double;
          break;
        case LTTV_TIME:
          TimeAdd(*value.v_time, *value.v_time, a->value.dv_time);
          break;
        case LTTV_POINTER:
          break;
        case LTTV_STRING:
          break;
        case LTTV_GOBJECT:
          break;
        case LTTV_NONE:
          break;
      }    
    }
  }
}


static void
attribute_interface_init (gpointer g_iface, gpointer iface_data)
{
  LttvIAttributeClass *klass = (LttvIAttributeClass *)g_iface;

  klass->get_number = (unsigned int (*) (LttvIAttribute *self)) 
      lttv_attribute_get_number;

  klass->named = (gboolean (*) (LttvIAttribute *self, gboolean *homogeneous))
      lttv_attribute_named;

  klass->get = (LttvAttributeType (*) (LttvIAttribute *self, unsigned i, 
      LttvAttributeName *name, LttvAttributeValue *v)) lttv_attribute_get;

  klass->get_by_name = (LttvAttributeType (*) (LttvIAttribute *self,
      LttvAttributeName name, LttvAttributeValue *v)) 
      lttv_attribute_get_by_name;

  klass->add = (LttvAttributeValue (*) (LttvIAttribute *self, 
      LttvAttributeName name, LttvAttributeType t)) lttv_attribute_add;

  klass->remove = (void (*) (LttvIAttribute *self, unsigned i)) 
      lttv_attribute_remove;

  klass->remove_by_name = (void (*) (LttvIAttribute *self,
      LttvAttributeName name)) lttv_attribute_remove_by_name;

  klass->find_subdir = (LttvIAttribute* (*) (LttvIAttribute *self, 
      LttvAttributeName name)) lttv_attribute_find_subdir;
}


static void
attribute_instance_init (GTypeInstance *instance, gpointer g_class)
{
  LttvAttribute *self = (LttvAttribute *)instance;
  self->names = g_hash_table_new(g_direct_hash, g_direct_equal);
  self->attributes = g_array_new(FALSE, FALSE, sizeof(Attribute));
}


static void
attribute_finalize (LttvAttribute *self)
{
  g_hash_table_destroy(self->names);
  g_critical("attribute_finalize()");
  g_array_free(self->attributes, TRUE);
  G_OBJECT_CLASS(g_type_class_peek_parent(
      g_type_class_peek(LTTV_ATTRIBUTE_TYPE)))->finalize(G_OBJECT(self));
}


static void
attribute_class_init (LttvAttributeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self))attribute_finalize;
}

GType 
lttv_attribute_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvAttributeClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) attribute_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvAttribute),
      0,      /* n_preallocs */
      (GInstanceInitFunc) attribute_instance_init    /* instance_init */
    };

    static const GInterfaceInfo iattribute_info = {
      (GInterfaceInitFunc) attribute_interface_init,    /* interface_init */
      NULL,                                       /* interface_finalize */
      NULL                                        /* interface_data */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "LttvAttributeType", &info, 
        0);
    g_type_add_interface_static (type, LTTV_IATTRIBUTE_TYPE, &iattribute_info);
  }
  return type;
}


