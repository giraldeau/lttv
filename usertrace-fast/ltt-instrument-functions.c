/****************************************************************************
 * ltt-instrument-functions.c
 *
 * Mathieu Desnoyers
 * March 2006
 */

#define LTT_TRACE
#define LTT_TRACE_FAST
#include <ltt/ltt-usertrace-fast.h>
#include <ltt/ltt-facility-user_generic.h>

void __attribute__((no_instrument_function)) __cyg_profile_func_enter (
		void *this_fn,
		void *call_site)
{
	/* don't care about the return value */
	trace_user_generic_function_entry(this_fn, call_site);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit (
		void *this_fn,
		void *call_site)
{
	/* don't care about the return value */
	trace_user_generic_function_exit(this_fn, call_site);
}

