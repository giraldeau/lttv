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

#ifndef TRACESET_H
#define TRACESET_H

#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <ltt/ltt.h>

/* A traceset is a set of traces to be analyzed together. */

typedef struct _LttvTraceset LttvTraceset;

typedef struct _LttvTrace LttvTrace;

/* Tracesets may be added to, removed from and their content listed. */

LttvTraceset *lttv_traceset_new();

char * lttv_traceset_name(LttvTraceset * s);

LttvTrace *lttv_trace_new(LttTrace *t);

LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig);

LttvTraceset *lttv_traceset_load(const gchar *filename);

gint lttv_traceset_save(LttvTraceset *s);

void lttv_traceset_destroy(LttvTraceset *s);

void lttv_trace_destroy(LttvTrace *t);

void lttv_traceset_add(LttvTraceset *s, LttvTrace *t);

unsigned lttv_traceset_number(LttvTraceset *s);

LttvTrace *lttv_traceset_get(LttvTraceset *s, unsigned i);

void lttv_traceset_remove(LttvTraceset *s, unsigned i);

/* An attributes table is attached to the set and to each trace in the set. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s);

LttvAttribute *lttv_trace_attribute(LttvTrace *t);

LttTrace *lttv_trace(LttvTrace *t);

guint lttv_trace_get_ref_number(LttvTrace * t);

guint lttv_trace_ref(LttvTrace * t);

guint lttv_trace_unref(LttvTrace * t);

#endif // TRACESET_H

