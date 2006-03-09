

#include <stdio.h>
#include <unistd.h>




void test_function(void)
{
	printf("we are in a test function\n");
}


int main(int argc, char **argv)
{
  printf("Abort with CTRL-C.\n");
  printf("See the result file in /tmp/ltt-usertrace.\n");


	while(1) {
		test_function();
		sleep(1);
	}
	
	return 0;
}

