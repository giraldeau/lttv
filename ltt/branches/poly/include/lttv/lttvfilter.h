#ifndef LTTV_FILTER_H
#define LTTV_FILTER_H

#include <ltt/ltt.h>


typedef struct _LttvTracesetSelector  LttvTracesetSelector;
typedef struct _LttvTraceSelector     LttvTraceSelector;
typedef struct _LttvTracefileSelector LttvTracefileSelector;


LttvTracesetSelector *lttv_traceset_selector_new(char * name);
LttvTraceSelector *lttv_trace_selector_new(LttTrace *t);
LttvTracefileSelector *lttv_tracefile_selector_new(LttTracefile *t);
void lttv_traceset_selector_destroy(LttvTracesetSelector *s);
void lttv_trace_selector_destroy(LttvTraceSelector *t);
void lttv_tracefile_selector_destroy(LttvTracefileSelector *t);


void lttv_traceset_selector_add(LttvTracesetSelector *s, LttvTraceSelector *t);
unsigned lttv_traceset_selector_number(LttvTracesetSelector *s);
LttvTraceSelector *lttv_traceset_selector_get(LttvTracesetSelector *s, unsigned i);
void lttv_traceset_selector_remove(LttvTracesetSelector *s, unsigned i);


void lttv_trace_selector_add(LttvTraceSelector *s, LttvTracefileSelector *t);
unsigned lttv_trace_selector_number(LttvTraceSelector *s);
LttvTracefileSelector *lttv_trace_selector_get(LttvTraceSelector *s, unsigned i);
void lttv_trace_selector_remove(LttvTraceSelector *s, unsigned i);

void lttv_trace_selector_set_selected(LttvTraceSelector *s, gboolean g);
void lttv_tracefile_selector_set_selected(LttvTracefileSelector *s, gboolean g);
gboolean lttv_trace_selector_get_selected(LttvTraceSelector *s);
gboolean lttv_tracefile_selector_get_selected(LttvTracefileSelector *s);
char * lttv_trace_selector_get_name(LttvTraceSelector *s);
char * lttv_tracefile_selector_get_name(LttvTracefileSelector *s);

#endif // LTTV_FILTER_H

