
/* Events and their content, including the raw data, are only valid 
   until reading another event from the same tracefile. 
   Indeed, since event reading is critical to the performance, 
   the memory associated with an event may be reused at each read. */


/* Obtain the tracefile unique integer id associated with the type of 
   this event */

unsigned ltt_event_eventtype_id(ltt_event *e);


/* Facility and type for the event */

ltt_facility *ltt_event_facility(ltt_event *e);

ltt_eventtype *ltt_event_eventtype(ltt_event *e);

ltt_field *ltt_event_field(ltt_event *e);

/* Time and cycle count for the event */

ltt_time ltt_event_time(ltt_event *e);

ltt_cycle_count ltt_event_cycle_count(ltt_event *e);


/* CPU id and system name of the event */

unsigned ltt_event_cpu_id(ltt_event *e);

char *ltt_event_system_name(ltt_event *e);


/* Pointer to the raw data for the event. This should not be used directly
   unless prepared to do all the architecture specific conversions. */

void *ltt_event_data(ltt_event *e);


/* The number of elements in a sequence field is specific to each event.
   This function returns the number of elements for an array or sequence
   field in an event. */

unsigned ltt_event_field_element_number(ltt_event *e, ltt_field *f);


/* Set the currently selected element for a sequence or array field. */

int ltt_event_field_element_select(ltt_event *e, ltt_field *f, unsigned i);


/* These functions extract data from an event after architecture specific
   conversions. */

unsigned ltt_event_get_unsigned(ltt_event *e, ltt_field *f);

int ltt_event_get_int(ltt_event *e, ltt_field *f);

unsigned long ltt_event_get_long_unsigned(ltt_event *e, ltt_field *f);

long int ltt_event_get_long_int(ltt_event *e, ltt_field *f);

float ltt_event_get_float(ltt_event *e, ltt_field *f);

double ltt_event_get_double(ltt_event *e, ltt_field *f);


/* The string obtained is only valid until the next read from
   the same tracefile. */

char *ltt_event_get_string(ltt_event *e, ltt_field *f);
