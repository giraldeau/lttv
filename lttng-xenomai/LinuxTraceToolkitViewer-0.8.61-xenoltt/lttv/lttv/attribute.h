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

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

/* FIXME : unnamed attributes not implemented */

#include <glib-object.h>
#include <lttv/iattribute.h>
#include <stdio.h>

#define LTTV_ATTRIBUTE_TYPE        (lttv_attribute_get_type ())
#define LTTV_ATTRIBUTE(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_ATTRIBUTE_TYPE, LttvAttribute))
#define LTTV_ATTRIBUTE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_ATTRIBUTE_TYPE, LttvAttributeClass))
#define LTTV_IS_ATTRIBUTE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_ATTRIBUTE_TYPE))
#define LTTV_IS_ATTRIBUTE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_ATTRIBUTE_TYPE))
#define LTTV_ATTRIBUTE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_ATTRIBUTE_TYPE, LttvAttributeClass))


typedef struct _LttvAttribute LttvAttribute;
typedef struct _LttvAttributeClass LttvAttributeClass;

struct _LttvAttribute {
  GObject parent;

  /* private members */
  GHashTable *names;
  GArray *attributes;
};

struct _LttvAttributeClass {
  GObjectClass parent;

};

GType lttv_attribute_get_type (void);


/* The functions exported in the IAttribute interface are also available
   directly. */


/* Total number of attributes */

unsigned int lttv_attribute_get_number(LttvAttribute *self);


/* Container type. Named (fields in struct or elements in a hash table)
   or unnamed (elements in an array) attributes, homogeneous type or not. */

gboolean lttv_attribute_named(LttvAttribute *self, gboolean *homogeneous);


/* Get the i th attribute along with its type and a pointer to its value. */

LttvAttributeType lttv_attribute_get(LttvAttribute *self, unsigned i, 
    LttvAttributeName *name, LttvAttributeValue *v, gboolean *is_named);
 

/* Get the named attribute in the table along with its type and a pointer to
   its value. If the named attribute does not exist, the type is LTTV_NONE. */

LttvAttributeType lttv_attribute_get_by_name(LttvAttribute *self,
    LttvAttributeName name, LttvAttributeValue *v);


/* Add an attribute, which must not exist. The name is an empty string for
   containers with unnamed attributes. */

LttvAttributeValue lttv_attribute_add(LttvAttribute *self, 
    LttvAttributeName name, LttvAttributeType t);

LttvAttributeValue lttv_attribute_add_unnamed(LttvAttribute *self, 
    LttvAttributeName name, LttvAttributeType t);

/* Remove an attribute */

void lttv_attribute_remove(LttvAttribute *self, unsigned i);

void lttv_attribute_remove_by_name(LttvAttribute *self,
    LttvAttributeName name);


/* Create an empty iattribute object and add it as an attribute under the
   specified name, or return an existing iattribute attribute. If an
   attribute of that name already exists but is not a GObject supporting the
   iattribute interface, return NULL. */

LttvAttribute* lttv_attribute_find_subdir(LttvAttribute *self, 
      LttvAttributeName name);

LttvAttribute* lttv_attribute_find_subdir_unnamed(LttvAttribute *self, 
      LttvAttributeName name);


gboolean lttv_attribute_find(LttvAttribute *self, LttvAttributeName name, 
    LttvAttributeType t, LttvAttributeValue *v);


/* Free recursively a tree of attributes. All contained gobject of type
   LttvAttribute are freed (unreferenced) recursively. */

// Now done by default.
// void lttv_attribute_recursive_free(LttvAttribute *self);

/* Add items from a tree of attributes to another tree. */

void lttv_attribute_recursive_add(LttvAttribute *dest, LttvAttribute *src);

void
lttv_attribute_write_xml(LttvAttribute *self, FILE *fp, int pos, int indent);

void lttv_attribute_read_xml(LttvAttribute *self, FILE *fp);

#endif // ATTRIBUTE_H
