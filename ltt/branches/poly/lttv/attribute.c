
#include <lttv/attribute.h>

typedef union _AttributeValue {
  int dv_int;
  unsigned dv_uint;
  long dv_long;
  unsigned long dv_ulong;
  float dv_float;
  double dv_double;
  timespec dv_timespec;
  gpointer dv_pointer;
  char *dv_string;
  gobject *dv_gobject;
} AttributeValue;


typedef struct _Attribute {
  LttvAttributeName name;
  LttvAttributeType type;
  AttributeValue value;
} Attribute;


GType 
lttv_attribute_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvAttributeClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      attribute_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvAttribute),
      0,      /* n_preallocs */
      attribute_instance_init    /* instance_init */
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
  *v = address_of_value(a->type, a->value);
  return a->type;
}


LttvAttributeType 
lttv_attribute_get_by_name(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeValue *v)
{
  Attribute *a;

  unsigned i;

  i = (unsigned)g_hash_table_lookup(self->names, (gconstpointer)name);
  if(i == 0) return LTTV_NONE;

  i--;
  a = &g_array_index(self->attributes, Attribute, i);
  *v = address_of_value(a->type, a->value);
  return a->type;
}


LttvAttributeValue 
lttv_attribute_add(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t)
{
  unsigned i;

  Attribute a, *pa;

  i = (unsigned)g_hash_table_lookup(self->names, (gconstpointer)name);
  if(i != 0) g_error("duplicate entry in attribute table");

  a->name = name;
  a->type = t;
  a->value = init_value(t);
  g_array_append_val(self->attributes, a);
  i = self->attributes->len - 1;
  pa = &g_array_index(self->attributes, Attribute, i)
  g_hash_table_insert(self->names, (gconstpointer)name, (gconstpointer)i + 1);
  return address_of_value(pa->value, t);
}


/* Remove an attribute */

void 
lttv_attribute_remove(LttvAttribute *self, unsigned i)
{
  Attribute *a;

  a = &g_array_index(self->attributes, Attribute, i);

  /* Remove the array element and its entry in the name index */

  g_hash_table_remove(self->names, (gconspointer)a->name);
  g_array_remove_index_fast(self->attributes, i);

  /* The element used to replace the removed element has its index entry
     all wrong now. Reinsert it with its new position. */

  g_hash_table_remove(self->names, (gconstpointer)a->name);
  g_hash_table_insert(self->names, (gconstpointer)a->name, i + 1);
}

void 
lttv_attribute_remove_by_name(LttvAttribute *self, LttvAttributeName name)
{
  unsigned i;

  i = (unsigned)g_hash_table_lookup(self->names, (gconstpointer)name);
  if(i == 0) g_error("remove by name non existent attribute");

  lttv_attribute_remove(self, i - 1);
}

/* Create an empty iattribute object and add it as an attribute under the
   specified name, or return an existing iattribute attribute. If an
   attribute of that name already exists but is not a GObject supporting the
   iattribute interface, return NULL. */

LttvIAttribute* 
lttv_attribute_create_subdir(LttvAttribute *self, LttvAttributeName name)
{
  unsigned i;

  Attribute a;

  LttvAttribute *new;

  i = (unsigned)g_hash_table_lookup(self->names, (gconstpointer)name);
  if(i != 0) {
    a = g_array_index(self->attributes, Attribute, i - 1);
    if(a->type == LTTV_GOBJECT && LTTV_IS_IATTRIBUTE(a->value->dv_gobject)) {
      return LTTV_IATTRIBUTE(a->value->dv_gobject);
    }
    else return NULL;    
  }
  new = g_object_new(LTTV_ATTRIBUTE_TYPE);
  *(lttv_attribute_add(self, name, LTTV_GOBJECT)->v_gobject) = new;
  return new;
}

gboolean 
lttv_attribute_find(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  unsigned i;

  Attribute *a;

  i = (unsigned)g_hash_table_lookup(self->names, (gconstpointer)name);
  if(i != 0) {
    a = &g_array_index(self->attributes, Attribute, i - 1);
    if(a->type != t) return FALSE;
    *v = address_of_value(a->value, t);
    return TRUE;
  }

  *v = lttv_attribute_add(self, name, t);
  return TRUE;
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

  klass->create_subdir = (LttvIAttribute* (*) (LttvIAttribute *self, 
      LttvAttributeName name)) lttv_attribute_create_subdir;
}


static guint
quark_hash(gconstpointer key)
{
  return (guint)key;
}


static gboolean
quark_equal(gconstpointer a, gconstpointer b)
{
  return (a == b)
}

static void
attribute_instance_init (GTypeInstance *instance, gpointer g_class)
{
  LttvAttribute *self = (LttvAttribute *)instance;
  self->names = g_hash_table_new(quark_hash, quark_equal);
  self->attributes = g_array_new(FALSE, FALSE, 
      sizeof(Attribute));
}


static void
attribute_finalize (LttvAttribute *self)
{
  g_hash_table_free(self->names);
  g_array_free(self->attributes, TRUE);
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_ATTRIBUTE_TYPE))->finalize(self);
}


static void
attribute_class_init (LttvAttributeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = attribute_finalize;
}

