
CC=gcc
INCLUDE_DIR=/usr/include
LIB_DIR=/usr/lib

all: sample-thread sample sample-highspeed sample-printf sample-instrument-fct ltt-instrument-functions.so.0

sample-thread: sample-thread.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^

sample: sample.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^
	
sample-highspeed: sample-highspeed.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^

sample-printf: sample-printf.c ltt-facility-loader-user_generic.c
	$(CC) $(CFLAGS) -o $@ $^

sample-instrument-fct: sample-instrument-fct.c ltt-facility-loader-user_generic.c ltt-instrument-functions.c
	$(CC) $(CFLAGS) -g -finstrument-functions -o $@ $^

ltt-instrument-functions.so.0: ltt-facility-loader-user_generic.c ltt-instrument-functions.c
	$(CC) $(CFLAGS) -shared -Wl,-soname,ltt-instrument-functions.so -o $@ $^
	ln -s ltt-instrument-functions.so.0 ltt-instrument-functions.so

.PHONY : clean install

install:
	if [ ! -e "$(INCLUDE_DIR)/ltt" ] ; then mkdir $(INCLUDE_DIR)/ltt ; fi
	cp -f ltt/*.h $(INCLUDE_DIR)/ltt
	cp -f ltt-instrument-functions.so* $(LIB_DIR)

clean:
	rm -fr *.o *~ sample-thread sample sample-highspeed sample-printf sample-instrument-fct ltt-instrument-functions.so.0 ltt-instrument-functions.so

