
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#include <ltt/ltt-facility-user_generic.h>


int main(int argc, char **argv)
{
	printf("Will trace the following string : Hello world! Have a nice day.\n");

	while(1) {
		trace_user_generic_string("Hello world! Have a nice day.");
		sleep(1);
	}
	
	return 0;
}

