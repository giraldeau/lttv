#ifndef _LTT_FACILITY_ID_USER_GENERIC_H_
#define _LTT_FACILITY_ID_USER_GENERIC_H_

#ifdef LTT_TRACE
#include <ltt/ltt-generic.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_user_generic_411B0F83;
extern ltt_facility_t ltt_facility_user_generic;


/****  event index  ****/

enum user_generic_event {
	event_user_generic_string,
	event_user_generic_string_pointer,
	facility_user_generic_num_events
};

#endif //LTT_TRACE
#endif //_LTT_FACILITY_ID_USER_GENERIC_H_
