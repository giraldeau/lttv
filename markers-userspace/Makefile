
# 32 bits arch makefile

CC=gcc

all: libtestlib.so testprog testprog.S

marker-lib.o: marker-lib.c marker.h
	$(CC) -O2 -DCONFIG_MARKERS -c -o $@ marker-lib.c

testprog: testprog.c marker.h marker-lib.o
	$(CC) -Wl,-Telf_i386.xmark -O2 -DCONFIG_MARKERS -L. -ltestlib -o $@ testprog.c marker-lib.o

testprog.S: testprog.c marker.h
	$(CC) -O2 -DCONFIG_MARKERS -S -o $@ testprog.c

libtestlib.so: testlib.c marker.h marker-lib.o
	$(CC) -Wl,-Telf_i386.xmark -O2 -DCONFIG_MARKERS -shared -o $@ testlib.c marker-lib.o

.PHONY: clean

clean:
	rm -f testprog testprog.S marker-lib.o libtestlib.so
