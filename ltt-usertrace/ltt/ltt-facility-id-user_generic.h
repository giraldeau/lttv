#ifndef _LTT_FACILITY_ID_USER_GENERIC_H_
#define _LTT_FACILITY_ID_USER_GENERIC_H_

#ifdef LTT_TRACE
#include <ltt/ltt-usertrace.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_user_generic_B1865E44;
extern ltt_facility_t ltt_facility_user_generic;


/****  event index  ****/

enum user_generic_event {
	event_user_generic_string,
	event_user_generic_string_pointer,
	event_user_generic_slow_printf,
	event_user_generic_function_entry,
	event_user_generic_function_exit,
	event_user_generic_thread_brand,
	facility_user_generic_num_events
};

#endif //LTT_TRACE
#endif //_LTT_FACILITY_ID_USER_GENERIC_H_
