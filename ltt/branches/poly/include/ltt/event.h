#ifndef EVENT_H
#define EVENT_H

#include <ltt/ltt.h>

/* Events and their content, including the raw data, are only valid 
   until reading another event from the same tracefile. 
   Indeed, since event reading is critical to the performance, 
   the memory associated with an event may be reused at each read. */


/* Obtain the trace unique integer id associated with the type of 
   this event */

unsigned ltt_event_eventtype_id(LttEvent *e);


/* Facility and type for the event */

LttFacility *ltt_event_facility(LttEvent *e);

LttEventType *ltt_event_eventtype(LttEvent *e);


/* Root field for the event */

LttField *ltt_event_field(LttEvent *e);


/* Time and cycle count for the event */

LttTime ltt_event_time(LttEvent *e);

LttCycleCount ltt_event_cycle_count(LttEvent *e);


/* Obtain the position of the event within the tracefile. This
   is used to seek back to this position later or to seek to another
   position, computed relative to this position. The event position
   structure is opaque and contains several fields, only two
   of which are user accessible: block number and event index
   within the block. */

void ltt_event_position(LttEvent *e, LttEventPosition *ep);

LttEventPosition * ltt_event_position_new();

void ltt_event_position_get(LttEventPosition *ep,
    unsigned *block_number, unsigned *index_in_block, LttTracefile ** tf);

void ltt_event_position_set(LttEventPosition *ep,
    unsigned block_number, unsigned index_in_block);


/* CPU id of the event */

unsigned ltt_event_cpu_id(LttEvent *e);


/* Pointer to the raw data for the event. This should not be used directly
   unless prepared to do all the architecture specific conversions. */

void *ltt_event_data(LttEvent *e);


/* The number of elements in a sequence field is specific to each event 
   instance. This function returns the number of elements for an array or 
   sequence field in an event. */

unsigned ltt_event_field_element_number(LttEvent *e, LttField *f);


/* Set the currently selected element for a sequence or array field. */

void ltt_event_field_element_select(LttEvent *e, LttField *f, unsigned i);


/* A union is like a structure except that only a single member at a time
   is present depending on the specific event instance. This function tells
   the active member for a union field in an event. */

unsigned ltt_event_field_union_member(LttEvent *e, LttField *f);


/* These functions extract data from an event after architecture specific
   conversions. */

unsigned ltt_event_get_unsigned(LttEvent *e, LttField *f);

int ltt_event_get_int(LttEvent *e, LttField *f);

unsigned long ltt_event_get_long_unsigned(LttEvent *e, LttField *f);

long int ltt_event_get_long_int(LttEvent *e, LttField *f);

float ltt_event_get_float(LttEvent *e, LttField *f);

double ltt_event_get_double(LttEvent *e, LttField *f);


/* The string obtained is only valid until the next read from
   the same tracefile. */

char *ltt_event_get_string(LttEvent *e, LttField *f);

#endif // EVENT_H
