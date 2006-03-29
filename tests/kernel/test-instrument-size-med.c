/* test-time-probe.c
 *
 * size of instrumented object.
 */


#define CONFIG_LTT_FACILITY_TESTS
#include "ltt-facility-tests.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>

void ltt_test_init(void)
{
	char mystring;
	
	trace_tests_write_string(&mystring);
	return;
}

