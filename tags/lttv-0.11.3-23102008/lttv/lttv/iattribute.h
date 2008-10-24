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

/* FIXME : unnamed attributes not implemented */

#ifndef IATTRIBUTE_H
#define IATTRIBUTE_H


#include <glib-object.h>
#include <ltt/time.h>

/* The content of a data structure may be seen as an array of pairs of
   attribute name and value. This simple model allows generic navigation 
   and access functions over a wide range of structures. The names are 
   represented by unique integer identifiers, GQuarks. */

/* Please note that adding a value of type gobject that is non null does not
 * increment the reference count of this object : the actual reference to
 * the object is "given" to the attribute tree. When the gobject value
 * is removed, the object is unreferenced. A value copy through
 * lttv_iattribute_copy_value does increase the reference count of the 
 * gobject. */

typedef GQuark LttvAttributeName;

typedef enum _LttvAttributeType {
  LTTV_INT, LTTV_UINT, LTTV_LONG, LTTV_ULONG, LTTV_FLOAT, LTTV_DOUBLE, 
  LTTV_TIME, LTTV_POINTER, LTTV_STRING, LTTV_GOBJECT, LTTV_NONE
} LttvAttributeType;

typedef union LttvAttributeValue {
  int *v_int;
  unsigned *v_uint;
  long *v_long;
  unsigned long *v_ulong;
  float *v_float;
  double *v_double;
  LttTime *v_time;
  gpointer *v_pointer;
  char **v_string;
  GObject **v_gobject;
} LttvAttributeValue;


/* GObject interface type macros */

#define LTTV_IATTRIBUTE_TYPE       (lttv_iattribute_get_type ())
#define LTTV_IATTRIBUTE(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_IATTRIBUTE_TYPE, LttvIAttribute))
#define LTTV_IATTRIBUTE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_IATTRIBUTE_TYPE, LttvIAttributeClass))
#define LTTV_IS_IATTRIBUTE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_IATTRIBUTE_TYPE))
#define LTTV_IS_IATTRIBUTE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_IATTRIBUTE_TYPE))
#define LTTV_IATTRIBUTE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), LTTV_IATTRIBUTE_TYPE, LttvIAttributeClass))


typedef struct _LttvIattribute LttvIAttribute; /* dummy object */
typedef struct _LttvIAttributeClass LttvIAttributeClass;


struct _LttvIAttributeClass {
  GTypeInterface parent;

  LttvIAttribute* (*new_attribute) (LttvIAttribute *self);

  unsigned int (*get_number) (LttvIAttribute *self);

  gboolean (*named) (LttvIAttribute *self, gboolean *homogeneous);

  LttvAttributeType (*get) (LttvIAttribute *self, unsigned i, 
      LttvAttributeName *name, LttvAttributeValue *v, gboolean *is_named);

  LttvAttributeType (*get_by_name) (LttvIAttribute *self,
      LttvAttributeName name, LttvAttributeValue *v);

  LttvAttributeValue (*add) (LttvIAttribute *self, LttvAttributeName name, 
      LttvAttributeType t);

  LttvAttributeValue (*add_unnamed) (LttvIAttribute *self,
			LttvAttributeName name,
      LttvAttributeType t);

  void (*remove) (LttvIAttribute *self, unsigned i);

  void (*remove_by_name) (LttvIAttribute *self,
      LttvAttributeName name);

  LttvIAttribute* (*find_subdir) (LttvIAttribute *self, 
      LttvAttributeName name);

  LttvIAttribute* (*find_subdir_unnamed) (LttvIAttribute *self, 
      LttvAttributeName name);

};


GType lttv_iattribute_get_type(void);


/* Total number of attributes */

unsigned int lttv_iattribute_get_number(LttvIAttribute *self);


/* Container type. Named (fields in struct or elements in a hash table)
   or unnamed (elements in an array) attributes, homogeneous type or not. */

gboolean lttv_iattribute_named(LttvIAttribute *self, gboolean *homogeneous);


/* Get the i th attribute along with its type and a pointer to its value. */

LttvAttributeType lttv_iattribute_get(LttvIAttribute *self, unsigned i, 
    LttvAttributeName *name, LttvAttributeValue *v, gboolean *is_named);
 

/* Get the named attribute in the table along with its type and a pointer to
   its value. If the named attribute does not exist, the type is LTTV_NONE. */

LttvAttributeType lttv_iattribute_get_by_name(LttvIAttribute *self,
    LttvAttributeName name, LttvAttributeValue *v);


/* Add an attribute, which must not exist. The name is an empty string for
   containers with unnamed attributes. Its value is initialized to 0 or NULL
   and its pointer returned. */

LttvAttributeValue lttv_iattribute_add(LttvIAttribute *self, 
    LttvAttributeName name, LttvAttributeType t);

LttvAttributeValue lttv_iattribute_add_unnamed(LttvIAttribute *self, 
    LttvAttributeName name, LttvAttributeType t);
/* Remove an attribute */

void lttv_iattribute_remove(LttvIAttribute *self, unsigned i);

void lttv_iattribute_remove_by_name(LttvIAttribute *self,
    LttvAttributeName name);


/* Create an empty iattribute object and add it as an attribute under the
   specified name, or return an existing iattribute attribute. If an
   attribute of that name already exists but is not a GObject supporting the
   iattribute interface, return NULL. */

LttvIAttribute* lttv_iattribute_find_subdir(LttvIAttribute *self, 
      LttvAttributeName name);

LttvIAttribute* lttv_iattribute_find_subdir_unnamed(LttvIAttribute *self, 
      LttvAttributeName name);

/* The remaining utility functions are not part of the LttvIAttribute
   interface but operate on objects implementing it. */

/* Find the named attribute in the table, which must be of the specified type.
   If it does not exist, it is created with a default value of 0 (NULL for
   pointer types). Since the address of the value is obtained, it may be
   changed easily afterwards. The function returns false when the attribute
   exists but is of incorrect type. */

gboolean lttv_iattribute_find(LttvIAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v);


/* Trees of attribute tables may be accessed using a hierarchical path with
   components separated by /, like in filesystems */

gboolean lttv_iattribute_find_by_path(LttvIAttribute *self, char *path, 
    LttvAttributeType t, LttvAttributeValue *v);


/* Shallow and deep copies */

void lttv_iattribute_copy_value(LttvAttributeType t, LttvAttributeValue dest, 
    LttvAttributeValue src);

LttvIAttribute *lttv_iattribute_shallow_copy(LttvIAttribute *self);

LttvIAttribute *lttv_iattribute_deep_copy(LttvIAttribute *self);

#endif // IATTRIBUTE_H
