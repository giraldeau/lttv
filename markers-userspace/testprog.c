#include <stdio.h>
#include <unistd.h>
#include "marker.h"

int main(int argc, char **argv)
{
	void *ptr;
	unsigned long val;

	while (1) {
		trace_mark(test_marker, "ptr %p val %lu", ptr, val);
		sleep(2);
	}
	return 0;
}
