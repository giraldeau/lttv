#ifndef _LTT_FACILITY_LOADER_USER_TESTS_H_
#define _LTT_FACILITY_LOADER_USER_TESTS_H_

#include <ltt/ltt-usertrace.h>
#include <ltt/ltt-facility-id-user_tests.h>

ltt_facility_t	ltt_facility_user_tests;
ltt_facility_t	ltt_facility_user_tests_CDD24456;

#define LTT_FACILITY_SYMBOL							ltt_facility_user_tests
#define LTT_FACILITY_CHECKSUM_SYMBOL		ltt_facility_user_tests_CDD24456
#define LTT_FACILITY_CHECKSUM						0xCDD24456
#define LTT_FACILITY_NAME								"user_tests"
#define LTT_FACILITY_NUM_EVENTS					facility_user_tests_num_events

#endif //_LTT_FACILITY_LOADER_USER_TESTS_H_
