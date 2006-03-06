

#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_BLOCKING 1
#include <ltt/ltt-facility-user_generic.h>


void test_function(void)
{
	printf("we are in a test function\n");
}


int main(int argc, char **argv)
{
	while(1) {
		test_function();
		sleep(1);
	}
	
	return 0;
}

