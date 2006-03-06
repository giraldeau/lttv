#ifndef _LTT_FACILITY_LOADER_USER_GENERIC_H_
#define _LTT_FACILITY_LOADER_USER_GENERIC_H_

#include <ltt/ltt-generic.h>
#include <ltt/ltt-facility-id-user_generic.h>

ltt_facility_t	ltt_facility_user_generic;
ltt_facility_t	ltt_facility_user_generic_FB850A80;

#define LTT_FACILITY_SYMBOL							ltt_facility_user_generic
#define LTT_FACILITY_CHECKSUM_SYMBOL		ltt_facility_user_generic_FB850A80
#define LTT_FACILITY_CHECKSUM						0xFB850A80
#define LTT_FACILITY_NAME								"user_generic"
#define LTT_FACILITY_NUM_EVENTS					facility_user_generic_num_events

#endif //_LTT_FACILITY_LOADER_USER_GENERIC_H_
