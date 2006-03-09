

#include <stdio.h>
#include <unistd.h>




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

