#ifndef _LTT_FACILITY_ID_USER_TESTS_H_
#define _LTT_FACILITY_ID_USER_TESTS_H_

#ifdef LTT_TRACE
#include <ltt/ltt-usertrace.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_user_tests_CDD24456;
extern ltt_facility_t ltt_facility_user_tests;


/****  event index  ****/

enum user_tests_event {
	event_user_tests_write_4bytes,
	facility_user_tests_num_events
};

#endif //LTT_TRACE
#endif //_LTT_FACILITY_ID_USER_TESTS_H_
