
CC=gcc
INCLUDE_DIR=/usr/include
LIB_DIR=/usr/lib
RANLIB=ranlib

all: libltt-instrument-functions.a libltt-instrument-functions.so.0 sample-thread sample sample-highspeed sample-printf sample-instrument-fct

sample-thread: sample-thread.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^

sample: sample.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^
	
sample-highspeed: sample-highspeed.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^

sample-printf: sample-printf.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^

sample-instrument-fct: sample-instrument-fct.c
	$(CC) $(CFLAGS) -L. -g -finstrument-functions -lltt-instrument-functions -o $@ $^

libltt-instrument-functions.a: ltt-instrument-functions.o ltt-facility-loader-user_generic.o
	@rm -f libltt-instrument-functions.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-instrument-functions.so.0: ltt-instrument-functions.o ltt-facility-loader-user_generic.o
	@rm -f libltt-instrument-functions.so libltt-instrument-functions.so.0
	$(CC) $(CFLAGS) -shared -Wl,-soname,libltt-instrument-functions.so -o $@ $^
	ln -s libltt-instrument-functions.so.0 libltt-instrument-functions.so

.PHONY : clean install

install:
	if [ ! -e "$(INCLUDE_DIR)/ltt" ] ; then mkdir $(INCLUDE_DIR)/ltt ; fi
	cp -f ltt/*.h $(INCLUDE_DIR)/ltt
	cp -f libltt-instrument-functions.so* libltt-instrument-functions.a $(LIB_DIR)

clean:
	rm -fr *.o *~ sample-thread sample sample-highspeed sample-printf sample-instrument-fct libltt-instrument-functions.so* libltt-instrument-functions.a

