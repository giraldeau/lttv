
#include <stdio.h>
#include <unistd.h>
#include <asm/timex.h>

#define LTT_TRACE
#define LTT_TRACE_FAST
#include <ltt/ltt-facility-user_tests.h>

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

/* Event logged : 4 bytes + 20 bytes header = 24 bytes. Let's use 1MB of
 * buffers. 1MB / 24bytes = 43690. So, if we write 20000 event, we should not
 * lose events. Check event lost count after tests. */

#define NR_LOOPS 20000

typedef unsigned long long cycles_t;

int main(int argc, char **argv)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t max_time = 0, min_time = 18446744073709551615ULL; /* (2^64)-1 */
	cycles_t tot_time = 0;

	for(i=0; i<NR_LOOPS; i++) {
		time1 = get_cycles();
		trace_user_tests_write_4bytes(5000);
		time2 = get_cycles();
		time = time2 - time1;
		max_time = max(max_time, time);
		min_time = min(min_time, time);
		tot_time += time;
	}
	
  printf("test results : time per probe (in cycles)\n");
  printf("number of loops : %d\n", NR_LOOPS);
  printf("total time : %llu\n", tot_time);
  printf("average time : %g\n", tot_time/(double)NR_LOOPS);
  printf("min : %llu\n", min_time);
  printf("max : %llu\n", max_time);
	
	return 0;
}

