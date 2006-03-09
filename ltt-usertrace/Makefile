
CC=gcc
INCLUDE_DIR=/usr/include
LIB_DIR=/usr/lib
RANLIB=ranlib

CFLAGS=-I. -O3
LDFLAGS=-L.

#For testing lib ltt-usertrace-fast
#CFLAGS+=-DLTT_SUBBUF_SIZE_CPU=134217728
#CFLAGS+=-DLTT_NULL_OUTPUT_TEST

all: libs samples


libs: libltt-instrument-functions.a libltt-instrument-functions.so.0 \
				libltt-usertrace-fast.a libltt-usertrace-fast.so.0

samples: sample-thread sample sample-highspeed sample-printf \
				sample-instrument-fct

sample-thread: sample-thread.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^

sample: sample.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^
	
sample-highspeed: sample-highspeed.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -L. -lltt-usertrace-fast -o $@ $^

sample-printf: sample-printf.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^

sample-instrument-fct: sample-instrument-fct.c
	$(CC) $(CFLAGS) -L. -g -finstrument-functions -lltt-instrument-functions -o $@ $^

libltt-usertrace-fast.a: ltt-usertrace-fast.o ltt-facility-loader-user_generic.o
	@rm -f libltt-usertrace-fast.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-usertrace-fast.so.0: ltt-usertrace-fast.o ltt-facility-loader-user_generic.o
	@rm -f libltt-usertrace-fast.so libltt-usertrace-fast.so.0
	$(CC) $(CFLAGS) -lpthread -shared -Wl,-soname,libltt-usertrace-fast.so -o $@ $^
	ln -s libltt-usertrace-fast.so.0 libltt-usertrace-fast.so

libltt-instrument-functions.a: ltt-instrument-functions.o ltt-facility-loader-user_generic.o ltt-usertrace-fast.o
	@rm -f libltt-instrument-functions.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-instrument-functions.so.0: ltt-instrument-functions.o ltt-facility-loader-user_generic.o ltt-usertrace-fast.o
	@rm -f libltt-instrument-functions.so libltt-instrument-functions.so.0
	$(CC) $(CFLAGS) -lpthread -shared -Wl,-soname,libltt-instrument-functions.so -o $@ $^
	ln -s libltt-instrument-functions.so.0 libltt-instrument-functions.so

.PHONY : clean install libs samples

install:
	if [ ! -e "$(INCLUDE_DIR)/ltt" ] ; then mkdir $(INCLUDE_DIR)/ltt ; fi
	cp -f ltt/*.h $(INCLUDE_DIR)/ltt
	cp -df libltt-instrument-functions.so* libltt-instrument-functions.a $(LIB_DIR)
	cp -df libltt-usertrace-fast.so* libltt-usertrace-fast.a $(LIB_DIR)

clean:
	rm -fr *.o *~ sample-thread sample sample-highspeed sample-printf sample-instrument-fct libltt-instrument-functions.so* libltt-instrument-functions.a libltt-usertrace-fast.a libltt-usertrace-fast.so.0

