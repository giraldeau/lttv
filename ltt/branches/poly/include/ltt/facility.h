#ifndef FACILITY_H
#define FACILITY_H

#include <ltt/ltt.h>

/* Facilities are obtained from an opened trace. The structures associated 
   with a facility are released when the trace is closed. Each facility
   is characterized by its name and checksum. */

char *ltt_facility_name(LttFacility *f);

LttChecksum ltt_facility_checksum(LttFacility *f);


/* Discover the event types within the facility. The event type integer id
   relative to the trace is from 0 to nb_event_types - 1. The event
   type id within the trace is the relative id + the facility base event
   id. */

unsigned ltt_facility_base_id(LttFacility *f);

unsigned ltt_facility_eventtype_number(LttFacility *f);

LttEventType *ltt_facility_eventtype_get(LttFacility *f, unsigned i);

LttEventType *ltt_facility_eventtype_get_by_name(LttFacility *f, char *name);

#endif // FACILITY_H
