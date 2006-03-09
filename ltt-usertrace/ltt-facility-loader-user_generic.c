/*
 * ltt-facility-loader-user_generic.c
 *
 * (C) Copyright  2005 - 
 *          Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Contains the LTT user space facility loader.
 *
 */


#define LTT_TRACE
#include <error.h>
#include <stdio.h>
#include <ltt/ltt-usertrace.h>
#include "ltt-facility-loader-user_generic.h"

static struct user_facility_info facility = {
	.name = LTT_FACILITY_NAME,
	.num_events = LTT_FACILITY_NUM_EVENTS,
#ifndef LTT_PACK
	.alignment = sizeof(void*),
#else
	.alignment = 0,
#endif //LTT_PACK
	.checksum = LTT_FACILITY_CHECKSUM,
	.int_size = sizeof(int),
	.long_size = sizeof(long),
	.pointer_size = sizeof(void*),
	.size_t_size = sizeof(size_t)
};

static void __attribute__((constructor)) __ltt_user_init(void)
{
	int err;
#ifdef LTT_SHOW_DEBUG
	printf("LTT : ltt-facility-user_generic init in userspace\n");
#endif //LTT_SHOW_DEBUG

	err = ltt_register_generic(&LTT_FACILITY_SYMBOL, &facility);
	LTT_FACILITY_CHECKSUM_SYMBOL = LTT_FACILITY_SYMBOL;
	
	if(err) {
#ifdef LTT_SHOW_DEBUG
		perror("Error in ltt_register_generic");
#endif //LTT_SHOW_DEBUG
	}
}

