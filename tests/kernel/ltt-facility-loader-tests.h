#ifndef _LTT_FACILITY_LOADER_TESTS_H_
#define _LTT_FACILITY_LOADER_TESTS_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include "ltt-facility-id-tests.h"

ltt_facility_t	ltt_facility_tests;
ltt_facility_t	ltt_facility_tests_2F06D8DB;

#define LTT_FACILITY_SYMBOL							ltt_facility_tests
#define LTT_FACILITY_CHECKSUM_SYMBOL		ltt_facility_tests_2F06D8DB
#define LTT_FACILITY_CHECKSUM						0x2F06D8DB
#define LTT_FACILITY_NAME								"tests"
#define LTT_FACILITY_NUM_EVENTS					facility_tests_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_TESTS_H_
