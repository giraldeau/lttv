#ifndef LTTV_FILTER_H
#define LTTV_FILTER_H

#include <ltt/ltt.h>


typedef struct _LttvTracesetSelector  LttvTracesetSelector;
typedef struct _LttvTraceSelector     LttvTraceSelector;
typedef struct _LttvTracefileSelector LttvTracefileSelector;
typedef struct _LttvEventtypeSelector LttvEventtypeSelector;


LttvTracesetSelector *lttv_traceset_selector_new(char * name);
LttvTraceSelector *lttv_trace_selector_new(LttTrace *t);
LttvTracefileSelector *lttv_tracefile_selector_new(LttTracefile *t);
LttvEventtypeSelector *lttv_eventtype_selector_new(LttEventType * et);
void lttv_traceset_selector_destroy(LttvTracesetSelector *s);
void lttv_trace_selector_destroy(LttvTraceSelector *t);
void lttv_tracefile_selector_destroy(LttvTracefileSelector *t);
void lttv_eventtype_selector_destroy(LttvEventtypeSelector *t);


void lttv_traceset_selector_trace_add(LttvTracesetSelector *s, 
				      LttvTraceSelector *t);
unsigned lttv_traceset_selector_trace_number(LttvTracesetSelector *s);
LttvTraceSelector *lttv_traceset_selector_trace_get(LttvTracesetSelector *s,
						    unsigned i);
void lttv_traceset_selector_trace_remove(LttvTracesetSelector *s,
					 unsigned i);


void lttv_trace_selector_tracefile_add(LttvTraceSelector *s,
				       LttvTracefileSelector *t);
unsigned lttv_trace_selector_tracefile_number(LttvTraceSelector *s);
LttvTracefileSelector *lttv_trace_selector_tracefile_get(LttvTraceSelector *s,
							 unsigned i);
void lttv_trace_selector_tracefile_remove(LttvTraceSelector *s, unsigned i);

void lttv_trace_selector_eventtype_add(LttvTraceSelector *s,
				       LttvEventtypeSelector *et);
unsigned lttv_trace_selector_eventtype_number(LttvTraceSelector *s);
LttvEventtypeSelector *lttv_trace_selector_eventtype_get(LttvTraceSelector *s,
							 unsigned i);
void lttv_trace_selector_eventtype_remove(LttvTraceSelector *s, unsigned i);


void lttv_tracefile_selector_eventtype_add(LttvTracefileSelector *s,
					   LttvEventtypeSelector *et);
unsigned lttv_tracefile_selector_eventtype_number(LttvTracefileSelector *s);
LttvEventtypeSelector *lttv_tracefile_selector_eventtype_get(LttvTracefileSelector *s,
							     unsigned i);
void lttv_tracefile_selector_eventtype_remove(LttvTracefileSelector *s, unsigned i);


void lttv_trace_selector_set_selected(LttvTraceSelector *s, gboolean g);
void lttv_tracefile_selector_set_selected(LttvTracefileSelector *s, gboolean g);
void lttv_eventtype_selector_set_selected(LttvEventtypeSelector *s, gboolean g);
gboolean lttv_trace_selector_get_selected(LttvTraceSelector *s);
gboolean lttv_tracefile_selector_get_selected(LttvTracefileSelector *s);
gboolean lttv_eventtype_selector_get_selected(LttvEventtypeSelector *s);
char * lttv_traceset_selector_get_name(LttvTracesetSelector *s);
char * lttv_trace_selector_get_name(LttvTraceSelector *s);
char * lttv_tracefile_selector_get_name(LttvTracefileSelector *s);
char * lttv_eventtype_selector_get_name(LttvEventtypeSelector *s);

LttvEventtypeSelector * lttv_eventtype_selector_clone(LttvEventtypeSelector * s);
void lttv_eventtype_selector_copy(LttvTraceSelector *s, LttvTracefileSelector *d);


#endif // LTTV_FILTER_H

