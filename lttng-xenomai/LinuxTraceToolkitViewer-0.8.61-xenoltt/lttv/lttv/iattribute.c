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

#include <lttv/iattribute.h>

static void
lttv_iattribute_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
  }
}


GType
lttv_iattribute_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvIAttributeClass),
      lttv_iattribute_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE, "LttvIAttribute", 
        &info, 0);
  }
  return type;
}


unsigned int lttv_iattribute_get_number(LttvIAttribute *self)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->get_number (self);
}


gboolean lttv_iattribute_named(LttvIAttribute *self, gboolean *homogeneous)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->named (self, homogeneous);
}


LttvAttributeType lttv_iattribute_get(LttvIAttribute *self, unsigned i, 
    LttvAttributeName *name, LttvAttributeValue *v, gboolean *is_named)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->get (self, i, name, v, is_named);
}
 

LttvAttributeType lttv_iattribute_get_by_name(LttvIAttribute *self,
    LttvAttributeName name, LttvAttributeValue *v)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->get_by_name (self, name, v);
}


LttvAttributeValue lttv_iattribute_add(LttvIAttribute *self, 
    LttvAttributeName name, LttvAttributeType t)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->add (self, name, t);
}

LttvAttributeValue lttv_iattribute_add_unnamed(LttvIAttribute *self, 
    LttvAttributeName name, LttvAttributeType t)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->add_unnamed (self, name, t);
}

void lttv_iattribute_remove(LttvIAttribute *self, unsigned i)
{
        return LTTV_IATTRIBUTE_GET_CLASS (self)->remove (self, i);
}


void lttv_iattribute_remove_by_name(LttvIAttribute *self,
    LttvAttributeName name)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->remove_by_name (self, name);
}

LttvIAttribute* lttv_iattribute_find_subdir(LttvIAttribute *self, 
      LttvAttributeName name)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->find_subdir (self, name);
}

LttvIAttribute* lttv_iattribute_find_subdir_unnamed(LttvIAttribute *self, 
      LttvAttributeName name)
{
  return LTTV_IATTRIBUTE_GET_CLASS (self)->find_subdir_unnamed (self, name);
}



/* Find the named attribute in the table, which must be of the specified type.
   If it does not exist, it is created with a default value of 0 (NULL for
   pointer types). Since the address of the value is obtained, it may be
   changed easily afterwards. The function returns false when the attribute
   exists but is of incorrect type. */

gboolean lttv_iattribute_find(LttvIAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  LttvAttributeType found_type;

  found_type = lttv_iattribute_get_by_name(self, name, v);
  if(found_type == t) return TRUE;

  if(found_type == LTTV_NONE) {
    *v = lttv_iattribute_add(self, name, t);
    return TRUE;
  }

  return FALSE;
}


/* Trees of attribute tables may be accessed using a hierarchical path with
   components separated by /, like in filesystems */

gboolean lttv_iattribute_find_by_path(LttvIAttribute *self, char *path, 
    LttvAttributeType t, LttvAttributeValue *v)
{
  LttvIAttribute *node = self;

  LttvAttributeType found_type;

  LttvAttributeName name;

  gchar **components, **cursor;

  components = g_strsplit(path, "\"", G_MAXINT);

  if(components == NULL || *components == NULL) {
    g_strfreev(components);
    return FALSE; 
  }

  for(cursor = components;;) {
    name = g_quark_from_string(*cursor);
    cursor++;

    if(*cursor == NULL) {
      g_strfreev(components);
      return lttv_iattribute_find(node, name, t, v);
    }
    else {
      found_type = lttv_iattribute_get_by_name(node, name, v);
      if(found_type == LTTV_NONE) {
        node = lttv_iattribute_find_subdir(node, name);
      }
      else if(found_type == LTTV_GOBJECT && 
	      LTTV_IS_IATTRIBUTE(*(v->v_gobject))) {
        node = LTTV_IATTRIBUTE(*(v->v_gobject));
      }
      else {
        g_strfreev(components);
        return FALSE;
      }
    }
  }
}


/* Shallow and deep copies */

LttvIAttribute *lttv_iattribute_shallow_copy(LttvIAttribute *self)
{
  LttvIAttribute *copy;

  LttvAttributeType t;

  LttvAttributeValue v, v_copy;

  LttvAttributeName name;

	gboolean is_named;

  int i;

  int nb_attributes = lttv_iattribute_get_number(self);

  copy = LTTV_IATTRIBUTE_GET_CLASS(self)->new_attribute(NULL);

  for(i = 0 ; i < nb_attributes ; i++) {
    t = lttv_iattribute_get(self, i, &name, &v, &is_named);
		if(is_named)
	    v_copy = lttv_iattribute_add(copy, name, t);
		else
	    v_copy = lttv_iattribute_add_unnamed(copy, name, t);
    lttv_iattribute_copy_value(t, v_copy, v);
  }
  return copy;
}

LttvIAttribute *lttv_iattribute_deep_copy(LttvIAttribute *self)
{
  LttvIAttribute *copy, *child;

  LttvAttributeType t;

  LttvAttributeValue v, v_copy;

  LttvAttributeName name;

	gboolean is_named;

  int i;

  int nb_attributes = lttv_iattribute_get_number(self);

  copy = LTTV_IATTRIBUTE_GET_CLASS(self)->new_attribute(NULL);

  for(i = 0 ; i < nb_attributes ; i++) {
    t = lttv_iattribute_get(self, i, &name, &v, &is_named);
		if(is_named)
    	v_copy = lttv_iattribute_add(copy, name, t);
		else
    	v_copy = lttv_iattribute_add_unnamed(copy, name, t);
    if(t == LTTV_GOBJECT && LTTV_IS_IATTRIBUTE(*(v.v_gobject))) {
      child = LTTV_IATTRIBUTE(*(v.v_gobject));
      *(v_copy.v_gobject) = G_OBJECT(lttv_iattribute_deep_copy(child));
    }
    else lttv_iattribute_copy_value(t, v_copy, v);
  }
  return copy;
}

void lttv_iattribute_copy_value(LttvAttributeType t, LttvAttributeValue dest, 
    LttvAttributeValue src) 
{
  switch(t) {
    case LTTV_INT:
      *(dest.v_int) = *(src.v_int); 
      break;

    case LTTV_UINT:
      *(dest.v_uint) = *(src.v_uint); 
      break;

    case LTTV_LONG:
      *(dest.v_long) = *(src.v_long); 
      break;

    case LTTV_ULONG:
      *(dest.v_ulong) = *(src.v_ulong); 
      break;

    case LTTV_FLOAT:
      *(dest.v_float) = *(src.v_float); 
      break;

    case LTTV_DOUBLE: 
      *(dest.v_double) = *(src.v_double); 
      break;

    case LTTV_TIME: 
      *(dest.v_time) = *(src.v_time); 
      break;

    case LTTV_POINTER:
      *(dest.v_pointer) = *(src.v_pointer); 
      break;

    case LTTV_STRING:
      *(dest.v_string) = *(src.v_string); 
      break;

    case LTTV_GOBJECT:
      *(dest.v_gobject) = *(src.v_gobject);
      if(*(dest.v_gobject) != NULL) g_object_ref(*(dest.v_gobject));
      break;

    case LTTV_NONE:
      break;
  }
}


