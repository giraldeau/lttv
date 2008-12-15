/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <lttv/attribute.h>
#include <ltt/ltt.h>
#include <ltt/compiler.h>

typedef union _AttributeValue {
  int dv_int;
  unsigned dv_uint;
  long dv_long;
  unsigned long dv_ulong;
  float dv_float;
  double dv_double;
  LttTime dv_time;
  gpointer dv_pointer;
  char *dv_string;
  GObject *dv_gobject;
} AttributeValue;


typedef struct _Attribute {
  LttvAttributeName name;
  LttvAttributeType type;
  AttributeValue value;
	gboolean is_named;
} Attribute;


static __inline__ LttvAttributeValue address_of_value(LttvAttributeType t,
                                                      AttributeValue *v)
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
  case LTTV_NONE: break;
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
  case LTTV_NONE: break;
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
    LttvAttributeValue *v, gboolean *is_named)
{
  Attribute *a;

  a = &g_array_index(self->attributes, Attribute, i);
  *name = a->name;
  *v = address_of_value(a->type, &(a->value));
	*is_named = a->is_named;
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
  unsigned int i;

  Attribute a, *pa;

  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(i != 0) g_error("duplicate entry in attribute table");

  a.name = name;
	a.is_named = 1;
  a.type = t;
  a.value = init_value(t);
  g_array_append_val(self->attributes, a);
  i = self->attributes->len - 1;
  pa = &g_array_index(self->attributes, Attribute, i);
  g_hash_table_insert(self->names, GUINT_TO_POINTER(name), 
      GUINT_TO_POINTER(i + 1));
  return address_of_value(t, &(pa->value));
}

LttvAttributeValue 
lttv_attribute_add_unnamed(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t)
{
  unsigned int i;

  Attribute a, *pa;

  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(i != 0) g_error("duplicate entry in attribute table");

  a.name = name;
	a.is_named = 0;
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

  /* If the element is a gobject, unreference it. */
  if(a->type == LTTV_GOBJECT && a->value.dv_gobject != NULL)
    g_object_unref(a->value.dv_gobject);
  
  /* Remove the array element and its entry in the name index */

  g_hash_table_remove(self->names, GUINT_TO_POINTER(a->name));
  g_array_remove_index_fast(self->attributes, i);

  /* The element used to replace the removed element has its index entry
     all wrong now. Reinsert it with its new position. */

  if(likely(self->attributes->len != i)){
    g_hash_table_remove(self->names, GUINT_TO_POINTER(a->name));
    g_hash_table_insert(self->names, GUINT_TO_POINTER(a->name), GUINT_TO_POINTER(i + 1));
  }
}

void 
lttv_attribute_remove_by_name(LttvAttribute *self, LttvAttributeName name)
{
  unsigned int i;

  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(unlikely(i == 0)) g_error("remove by name non existent attribute");

  lttv_attribute_remove(self, i - 1);
}

/* Create an empty iattribute object and add it as an attribute under the
   specified name, or return an existing iattribute attribute. If an
   attribute of that name already exists but is not a GObject supporting the
   iattribute interface, return NULL. */

/*CHECK*/LttvAttribute* 
lttv_attribute_find_subdir(LttvAttribute *self, LttvAttributeName name)
{
  unsigned int i;

  Attribute a;

  LttvAttribute *new;
  
  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(likely(i != 0)) {
    a = g_array_index(self->attributes, Attribute, i - 1);
    if(likely(a.type == LTTV_GOBJECT && LTTV_IS_IATTRIBUTE(a.value.dv_gobject))) {
      return LTTV_ATTRIBUTE(a.value.dv_gobject);
    }
    else return NULL;    
  }
  new = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  *(lttv_attribute_add(self, name, LTTV_GOBJECT).v_gobject) = G_OBJECT(new);
  return (LttvAttribute *)new;
}

/*CHECK*/LttvAttribute* 
lttv_attribute_find_subdir_unnamed(LttvAttribute *self, LttvAttributeName name)
{
  unsigned int i;

  Attribute a;

  LttvAttribute *new;
  
  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(likely(i != 0)) {
    a = g_array_index(self->attributes, Attribute, i - 1);
    if(likely(a.type == LTTV_GOBJECT && LTTV_IS_IATTRIBUTE(a.value.dv_gobject))) {
      return LTTV_ATTRIBUTE(a.value.dv_gobject);
    }
    else return NULL;    
  }
  new = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  *(lttv_attribute_add_unnamed(self, name, LTTV_GOBJECT).v_gobject)
		= G_OBJECT(new);
  return (LttvAttribute *)new;
}

gboolean 
lttv_attribute_find(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  unsigned int i;

  Attribute *a;

  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(likely(i != 0)) {
    a = &g_array_index(self->attributes, Attribute, i - 1);
    if(unlikely(a->type != t)) return FALSE;
    *v = address_of_value(t, &(a->value));
    return TRUE;
  }

  *v = lttv_attribute_add(self, name, t);
  return TRUE;
}

gboolean 
lttv_attribute_find_unnamed(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  unsigned i;

  Attribute *a;

  i = GPOINTER_TO_UINT(g_hash_table_lookup(self->names, GUINT_TO_POINTER(name)));
  if(likely(i != 0)) {
    a = &g_array_index(self->attributes, Attribute, i - 1);
    if(unlikely(a->type != t)) return FALSE;
    *v = address_of_value(t, &(a->value));
    return TRUE;
  }

  *v = lttv_attribute_add_unnamed(self, name, t);
  return TRUE;
}


/*void lttv_attribute_recursive_free(LttvAttribute *self)
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
}*/


void lttv_attribute_recursive_add(LttvAttribute *dest, LttvAttribute *src)
{
  int i, nb;

  Attribute *a;

  LttvAttributeValue value;

  nb = src->attributes->len;

  for(i = 0 ; i < nb ; i++) {
    a = &g_array_index(src->attributes, Attribute, i);
    if(a->type == LTTV_GOBJECT && LTTV_IS_ATTRIBUTE(a->value.dv_gobject)) {
			if(a->is_named)
	      lttv_attribute_recursive_add(
  	    /*CHECK*/(LttvAttribute *)lttv_attribute_find_subdir(dest, a->name),
    	      (LttvAttribute *)(a->value.dv_gobject));
			else
	      lttv_attribute_recursive_add(
  	    /*CHECK*/(LttvAttribute *)lttv_attribute_find_subdir_unnamed(
						dest, a->name), (LttvAttribute *)(a->value.dv_gobject));
    }
    else {
			if(a->is_named)
	      g_assert(lttv_attribute_find(dest, a->name, a->type, &value));
			else
	      g_assert(lttv_attribute_find_unnamed(dest, a->name, a->type, &value));
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
          *value.v_time = ltt_time_add(*value.v_time, a->value.dv_time);
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
print_indent(FILE *fp, int pos)
{
  int i;

  for(i = 0 ; i < pos ; i++) putc(' ', fp);
}


void 
lttv_attribute_write_xml(LttvAttribute *self, FILE *fp, int pos, int indent)
{
  int i, nb;

  Attribute *a;

  nb = self->attributes->len;

  fprintf(fp,"<ATTRS>\n");
  for(i = 0 ; i < nb ; i++) {
    a = &g_array_index(self->attributes, Attribute, i);
    print_indent(fp, pos);
    fprintf(fp, "<ATTR NAME=\"%s\" ", g_quark_to_string(a->name));
    if(a->type == LTTV_GOBJECT && LTTV_IS_ATTRIBUTE(a->value.dv_gobject)) {
      fprintf(fp, "TYPE=ATTRS>");
      lttv_attribute_write_xml((LttvAttribute *)(a->value.dv_gobject), fp,
          pos + indent, indent);
    }
    else {
      switch(a->type) {
	case LTTV_INT:
          fprintf(fp, "TYPE=INT VALUE=%d/>\n", a->value.dv_int);
          break;
        case LTTV_UINT:
          fprintf(fp, "TYPE=UINT VALUE=%u/>\n", a->value.dv_uint);
          break;
        case LTTV_LONG:
          fprintf(fp, "TYPE=LONG VALUE=%ld/>\n", a->value.dv_long);
          break;
        case LTTV_ULONG:
          fprintf(fp, "TYPE=ULONG VALUE=%lu/>\n", a->value.dv_ulong);
          break;
        case LTTV_FLOAT:
          fprintf(fp, "TYPE=FLOAT VALUE=%f/>\n", a->value.dv_float);
          break;
        case LTTV_DOUBLE:
          fprintf(fp, "TYPE=DOUBLE VALUE=%f/>\n", a->value.dv_double);
          break;
        case LTTV_TIME:
          fprintf(fp, "TYPE=TIME SEC=%lu NSEC=%lu/>\n", a->value.dv_time.tv_sec,
              a->value.dv_time.tv_nsec);
          break;
        case LTTV_POINTER:
          fprintf(fp, "TYPE=POINTER VALUE=%p/>\n", a->value.dv_pointer);
          break;
        case LTTV_STRING:
          fprintf(fp, "TYPE=STRING VALUE=\"%s\"/>\n", a->value.dv_string);
          break;
        case LTTV_GOBJECT:
          fprintf(fp, "TYPE=GOBJECT VALUE=%p/>\n", a->value.dv_gobject);
          break;
        case LTTV_NONE:
          fprintf(fp, "TYPE=NONE/>\n");
          break;
      }    
    }
  }
  print_indent(fp, pos);
  fprintf(fp,"</ATTRS>\n");
}


void 
lttv_attribute_read_xml(LttvAttribute *self, FILE *fp)
{
  int res;

  char buffer[256], type[10];

  LttvAttributeName name;

  LttvAttributeValue value;

  LttvAttribute *subtree;

  fscanf(fp,"<ATTRS>");
  while(1) {
    res = fscanf(fp, "<ATTR NAME=\"%256[^\"]\" TYPE=%10[^ >]", buffer, type);
    g_assert(res == 2);
    name = g_quark_from_string(buffer);
    if(strcmp(type, "ATTRS") == 0) {
      fscanf(fp, ">");
      subtree = lttv_attribute_find_subdir(self, name);
      lttv_attribute_read_xml(subtree, fp);
    }
    else if(strcmp(type, "INT") == 0) {
      value = lttv_attribute_add(self, name, LTTV_INT);
      res = fscanf(fp, " VALUE=%d/>", value.v_int);
      g_assert(res == 1);
    }
    else if(strcmp(type, "UINT") == 0) {
      value = lttv_attribute_add(self, name, LTTV_UINT);
      res = fscanf(fp, " VALUE=%u/>", value.v_uint);
      g_assert(res == 1);
    }
    else if(strcmp(type, "LONG") == 0) {
      value = lttv_attribute_add(self, name, LTTV_LONG);
      res = fscanf(fp, " VALUE=%ld/>", value.v_long);
      g_assert(res == 1);
    }
    else if(strcmp(type, "ULONG") == 0) {
      value = lttv_attribute_add(self, name, LTTV_ULONG);
      res = fscanf(fp, " VALUE=%lu/>", value.v_ulong);
      g_assert(res == 1);
    }
    else if(strcmp(type, "FLOAT") == 0) {
      float d;
      value = lttv_attribute_add(self, name, LTTV_FLOAT);
      res = fscanf(fp, " VALUE=%f/>", &d);
      *(value.v_float) = d;
      g_assert(res == 1);
    }
    else if(strcmp(type, "DOUBLE") == 0) {
      value = lttv_attribute_add(self, name, LTTV_DOUBLE);
      res = fscanf(fp, " VALUE=%lf/>", value.v_double);
      g_assert(res == 1);
    }
    else if(strcmp(type, "TIME") == 0) {
      value = lttv_attribute_add(self, name, LTTV_TIME);
      res = fscanf(fp, " SEC=%lu NSEC=%lu/>", &(value.v_time->tv_sec), 
          &(value.v_time->tv_nsec));
      g_assert(res == 2);
    }
    else if(strcmp(type, "POINTER") == 0) {
      value = lttv_attribute_add(self, name, LTTV_POINTER);
      res = fscanf(fp, " VALUE=%p/>", value.v_pointer);
      g_error("Cannot read a pointer");
    }
    else if(strcmp(type, "STRING") == 0) {
      value = lttv_attribute_add(self, name, LTTV_STRING);
      res = fscanf(fp, " VALUE=\"%256[^\"]\"/>", buffer);
      *(value.v_string) = g_strdup(buffer);
      g_assert(res == 1);
    }
    else if(strcmp(type, "GOBJECT") == 0) {
      value = lttv_attribute_add(self, name, LTTV_GOBJECT);
      res = fscanf(fp, " VALUE=%p/>", value.v_gobject);
      g_error("Cannot read a pointer");
    }
    else if(strcmp(type, "NONE") == 0) {
      value = lttv_attribute_add(self, name, LTTV_NONE);
      fscanf(fp, "/>");
    }
    else g_error("Unknown type to read");
  }
  fscanf(fp,"</ATTRS>");
}

static LttvAttribute *
new_attribute (LttvAttribute *self)
{
  return g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
}


static void
attribute_interface_init (gpointer g_iface, gpointer iface_data)
{
  LttvIAttributeClass *klass = (LttvIAttributeClass *)g_iface;

  klass->new_attribute = (LttvIAttribute* (*) (LttvIAttribute *self))
      new_attribute;

  klass->get_number = (unsigned int (*) (LttvIAttribute *self)) 
      lttv_attribute_get_number;

  klass->named = (gboolean (*) (LttvIAttribute *self, gboolean *homogeneous))
      lttv_attribute_named;

  klass->get = (LttvAttributeType (*) (LttvIAttribute *self, unsigned i, 
      LttvAttributeName *name, LttvAttributeValue *v, gboolean *is_named)) 
			lttv_attribute_get;

  klass->get_by_name = (LttvAttributeType (*) (LttvIAttribute *self,
      LttvAttributeName name, LttvAttributeValue *v)) 
      lttv_attribute_get_by_name;

  klass->add = (LttvAttributeValue (*) (LttvIAttribute *self, 
      LttvAttributeName name, LttvAttributeType t)) lttv_attribute_add;

  klass->add_unnamed = (LttvAttributeValue (*) (LttvIAttribute *self, 
      LttvAttributeName name, LttvAttributeType t)) lttv_attribute_add_unnamed;

  klass->remove = (void (*) (LttvIAttribute *self, unsigned i)) 
      lttv_attribute_remove;

  klass->remove_by_name = (void (*) (LttvIAttribute *self,
      LttvAttributeName name)) lttv_attribute_remove_by_name;

  klass->find_subdir = (LttvIAttribute* (*) (LttvIAttribute *self, 
      LttvAttributeName name)) lttv_attribute_find_subdir;

  klass->find_subdir = (LttvIAttribute* (*) (LttvIAttribute *self, 
      LttvAttributeName name)) lttv_attribute_find_subdir_unnamed;
}

static void
attribute_instance_init (GTypeInstance *instance, gpointer g_class)
{
  LttvAttribute *self = (LttvAttribute *)instance;
  self->names = g_hash_table_new(g_direct_hash,
                                 g_direct_equal);
  self->attributes = g_array_new(FALSE, FALSE, sizeof(Attribute));
}


static void
attribute_finalize (LttvAttribute *self)
{
  guint i;
  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "attribute_finalize()");

  for(i=0;i<self->attributes->len;i++) {
    lttv_attribute_remove(self, i);
  }
  
  g_hash_table_destroy(self->names);
  g_array_free(self->attributes, TRUE);
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
      (GInstanceInitFunc) attribute_instance_init,    /* instance_init */
      NULL    /* value handling */
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


