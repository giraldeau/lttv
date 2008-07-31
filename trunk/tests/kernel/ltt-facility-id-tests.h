#ifndef _LTT_FACILITY_ID_TESTS_H_
#define _LTT_FACILITY_ID_TESTS_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_tests_CA7F1536;
extern ltt_facility_t ltt_facility_tests;


/****  event index  ****/

enum tests_event {
	event_tests_write_4bytes,
	facility_tests_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_TESTS_H_
