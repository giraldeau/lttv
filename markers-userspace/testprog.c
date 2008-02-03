#include <stdio.h>
#include "marker.h"

int main(int argc, char **argv)
{
	void *ptr;
	unsigned long val;

	trace_mark(test_marker, "ptr %p val %lu", ptr, val);
	return 0;
}
