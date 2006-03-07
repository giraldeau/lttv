

CC=gcc

test: test.c ltt-usertrace-fast.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^

.PHONY : clean

clean:
	rm -fr *.o *~ test
