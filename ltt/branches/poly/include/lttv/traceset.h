#ifndef TRACESET_H
#define TRACESET_H

#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <ltt/ltt.h>

/* A traceset is a set of traces to be analyzed together. */

typedef struct _LttvTraceset LttvTraceset;


/* Tracesets may be added to, removed from and their content listed. */

LttvTraceset *lttv_traceset_new();

LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig);

LttvTraceset *lttv_traceset_load(const gchar *filename);

gint lttv_traceset_save(LttvTraceset *s);

void lttv_traceset_destroy(LttvTraceset *s);

void lttv_traceset_add(LttvTraceset *s, LttTrace *t);

unsigned lttv_traceset_number(LttvTraceset *s);

LttTrace *lttv_traceset_get(LttvTraceset *s, unsigned i);

void lttv_traceset_remove(LttvTraceset *s, unsigned i);

/* An attributes table is attached to the set and to each trace in the set. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s);

LttvAttribute *lttv_traceset_trace_attribute(LttvTraceset *s, 
    unsigned i);

#endif // TRACESET_H

