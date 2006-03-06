
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_BLOCKING 1
#include <ltt/ltt-facility-user_generic.h>
#include <ltt/ltt-facility-custom-user_generic.h>


int main(int argc, char **argv)
{
	printf("Will trace a printf of an incrementing counter.\n");
	unsigned int count = 0;

	while(1) {
		trace_user_generic_slow_printf("Counter value is : %u.", count);
		count++;
		sleep(1);
	}
	
	return 0;
}

