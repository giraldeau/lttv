
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_BLOCKING 1
#include <ltt/ltt-facility-user_generic.h>


int main(int argc, char **argv)
{
	printf("Will create a branded thread\n");
	trace_user_generic_thread_brand("Sample_brand");
	
	sleep(2);
	
	return 0;
}

