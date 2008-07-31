
CC=gcc
INCLUDE_DIR=/usr/include
LIB_DIR=/usr/lib
RANLIB=ranlib

CFLAGS=-I. -O2 -L.

#For testing lib ltt-usertrace-fast
#CFLAGS+=-DLTT_SUBBUF_SIZE_CPU=134217728
#CFLAGS+=-DLTT_NULL_OUTPUT_TEST

all: tests

#TEST PROGRAMS

tests: test-usertrace-time test-usertrace-time-slow

test-usertrace-time: test-usertrace-time.c ltt-facility-loader-user_tests.c
	$(CC) $(CFLAGS) -lltt-usertrace-fast -o $@ $^

test-usertrace-time-slow: test-usertrace-time-slow.c ltt-facility-loader-user_tests.c
	$(CC) $(CFLAGS) -o $@ $^

#LIBRAIRIES

.PHONY : clean tests

clean:
	rm -fr *.o *~ test-usertrace-time

