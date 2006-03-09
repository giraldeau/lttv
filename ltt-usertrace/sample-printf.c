
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_BLOCKING 1
#include <ltt/ltt-facility-user_generic.h>
#include <ltt/ltt-facility-custom-user_generic.h>
// Notice the inclusion of ltt-facility-custom-user_generic.h for the
// slow_printf support


int main(int argc, char **argv)
{
	printf("Will trace a printf of an incrementing counter.\n");
	printf("Abort with CTRL-C.\n");
	printf("No file is created with this example : it logs through a kernel\n");
	printf("system call. See the LTTng lttctl command to start tracing.\n");

	unsigned int count = 0;

	while(1) {
		trace_user_generic_slow_printf("in: %s at: %s:%d: Counter value is: %u.",
																	__FILE__, __func__, __LINE__, count);
		count++;
		sleep(1);
	}
	
	return 0;
}

