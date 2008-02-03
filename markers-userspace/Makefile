
CC=gcc

all: testprog testprog.S

marker-lib.o: marker-lib.c marker.h
	$(CC) -O2 -DCONFIG_MARKERS -c -o $@ marker-lib.c

testprog: testprog.c marker.h marker-lib.o
	$(CC) -O2 -DCONFIG_MARKERS -o $@ testprog.c marker-lib.o

testprog.S: testprog.c marker.h
	$(CC) -O2 -DCONFIG_MARKERS -S -o $@ testprog.c

.PHONY: clean

clean:
	rm -f testprog testprog.S
