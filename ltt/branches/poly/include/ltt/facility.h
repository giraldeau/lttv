#ifndef FACILITY_H
#define FACILITY_H

#include <ltt/ltt.h>

/* Facilities are obtained from an opened trace. The structures associated 
   with a facility are released when the trace is closed. Each facility
   is characterized by its name and checksum. */

char *ltt_facility_name(ltt_facility *f);

ltt_checksum ltt_facility_checksum(ltt_facility *f);


/* Discover the event types within the facility. The event type integer id
   relative to the trace is from 0 to nb_event_types - 1. The event
   type id within the trace is the relative id + the facility base event
   id. */

unsigned ltt_facility_base_id(ltt_facility *f);

unsigned ltt_facility_eventtype_number(ltt_facility *f);

ltt_eventtype *ltt_facility_eventtype_get(ltt_facility *f, unsigned i);

ltt_eventtype *ltt_facility_eventtype_get_by_name(ltt_facility *f, char *name);

#endif // FACILITY_H
