#ifndef FACILITY_H
#define FACILITY_H

#include <ltt/ltt.h>

/* A facility is obtained from a .event file containing event type
   declarations. The facility content must have the specified checksum.
   The structures associated with a facility may be released with
   a call to ltt_close_facility if its usage count is 0. */

ltt_facility *ltt_facility_open(char *pathname, ltt_checksum c);

int ltt_facility_close(ltt_facility *f);


/* Obtain the name and checksum of the facility */

char *ltt_facility_name(ltt_facility *f);

ltt_checksum ltt_facility_checksum(ltt_facility *f);


/* Discover the event types within the facility. The event type integer id
   used here is specific to the trace (from 0 to nb_event_types - 1). */

unsigned ltt_facility_eventtype_number(ltt_facility *f);

ltt_eventtype *ltt_facility_eventtype_get(ltt_facility *f, unsigned i);

ltt_eventtype *ltt_facility_eventtype_get_by_name(ltt_facility *f, char *name);

#endif // FACILITY_H
