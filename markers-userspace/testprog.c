#include <stdio.h>
#include <unistd.h>
#include "marker.h"

extern void testfct(void);

int main(int argc, char **argv)
{
	void *ptr;
	unsigned long val;

	while (1) {
		trace_mark(test_marker, "ptr %p val %lu", ptr, val);
		testfct();
		sleep(2);
	}
	return 0;
}
