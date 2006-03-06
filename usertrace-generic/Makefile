

CC=gcc

all: sample-thread sample sample-highspeed sample-printf

sample-thread: sample-thread.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -lpthread -o $@ $^

sample: sample.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -o $@ $^
	
sample-highspeed: sample-highspeed.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -o $@ $^

sample-printf: sample-printf.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -I. -o $@ $^

.PHONY : clean

clean:
	rm -fr *.o *~ sample-thread sample sample-highspeed sample-printf

