
RANLIB=ranlib

CC=gcc

all: test sample-instrument-fct libltt-instrument-functions.a libltt-instrument-functions.so.0

test: test.c ltt-usertrace-fast.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^


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

install:
	if [ ! -e "$(INCLUDE_DIR)/ltt" ] ; then mkdir $(INCLUDE_DIR)/ltt ; fi
	cp -f ltt/*.h $(INCLUDE_DIR)/ltt
	cp -df libltt-instrument-functions.so* libltt-instrument-functions.a $(LIB_DIR)

.PHONY : clean install

clean:
	rm -fr *.o *~ test sample-instrument-fct libltt-instrument-functions.a libltt-instrument-functions.so*
