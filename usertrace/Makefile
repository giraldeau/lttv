

CC=gcc

test: test.c lttng_usertrace.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^

.PHONY : clean

clean:
	rm -fr *.o *~ test
