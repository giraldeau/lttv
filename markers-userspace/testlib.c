#include <stdio.h>
#include "marker.h"

void testfct(void)
{
	void *ptr = (void *)0x2;

	trace_mark(test_lib_mark, "%p", ptr);
}
