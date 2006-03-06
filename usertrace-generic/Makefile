

CC=gcc

all: sample-thread sample

sample-thread: sample-thread.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -lpthread -o $@ $^

sample: sample.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -o $@ $^

.PHONY : clean

clean:
	rm -fr *.o *~ sample-thread sample

