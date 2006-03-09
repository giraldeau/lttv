
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_TRACE_FAST
#include <ltt/ltt-facility-user_generic.h>


int main(int argc, char **argv)
{
	printf("Will trace the following string : Running fast! in an infinite loop.\n");
	printf("Abort with CTRL-C or it will quickly fill up your disk.\n");
	printf("See the result file in /tmp/ltt-usertraces.\n");

	while(1) {
		trace_user_generic_string("Running fast!");
	}
	
	return 0;
}

