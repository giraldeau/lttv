
CC=gcc
INCLUDE_DIR=/usr/include
LIB_DIR?=/usr/lib
RANLIB=ranlib

LTT_CFLAGS=-I. -O2 -L. -fPIC
# note : x86_64 needs -fPIC ? FIXME

#For testing lib ltt-usertrace-fast
#CFLAGS+=-DLTT_SUBBUF_SIZE_CPU=134217728
#CFLAGS+=-DLTT_NULL_OUTPUT_TEST

all: libs samples

#SAMPLE PROGRAMS

samples: sample sample-highspeed sample-printf \
	sample-instrument-fct sample-thread-slow sample-thread-fast sample-thread-brand sample-block

sample: sample.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lltt-loader-user_generic -o $@ $^
sample-block: sample-block.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lltt-loader-user_generic -o $@ $^
	
sample-thread-brand: sample-thread-brand.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lltt-loader-user_generic -o $@ $^

sample-highspeed: sample-highspeed.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lltt-usertrace-fast -lltt-loader-user_generic -o $@ $^

sample-printf: sample-printf.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lltt-loader-user_generic -o $@ $^

sample-instrument-fct: sample-instrument-fct.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -g -finstrument-functions -lltt-instrument-functions -o $@ $^

sample-thread-slow: sample-thread-slow.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lpthread -lltt-loader-user_generic -o $@ $^

sample-thread-fast: sample-thread-fast.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lpthread -lltt-usertrace-fast -lltt-loader-user_generic -o $@ $^


#LIBRAIRIES

libs: libltt-instrument-functions.a libltt-instrument-functions.so.0 \
				libltt-usertrace-fast.a libltt-usertrace-fast.so.0 \
				libltt-loader-user_generic.a libltt-loader-user_generic.so.0

libltt-usertrace-fast.a: ltt-usertrace-fast.o
	@rm -f libltt-usertrace-fast.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-usertrace-fast.so.0: ltt-usertrace-fast.o
	@rm -f libltt-usertrace-fast.so libltt-usertrace-fast.so.0
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lpthread -shared -Wl,-soname,libltt-usertrace-fast.so -o $@ $^
	ln -s libltt-usertrace-fast.so.0 libltt-usertrace-fast.so

libltt-instrument-functions.a: ltt-instrument-functions.o ltt-facility-loader-user_generic.o ltt-usertrace-fast.o
	@rm -f libltt-instrument-functions.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-instrument-functions.so.0: ltt-instrument-functions.o ltt-facility-loader-user_generic.o ltt-usertrace-fast.o
	@rm -f libltt-instrument-functions.so libltt-instrument-functions.so.0
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lpthread -shared -Wl,-soname,libltt-instrument-functions.so -o $@ $^
	ln -s libltt-instrument-functions.so.0 libltt-instrument-functions.so

libltt-loader-user_generic.a: ltt-facility-loader-user_generic.o
	@rm -f libltt-loader-user_generic.a
	$(AR) rc $@ $^
	$(RANLIB) $@

libltt-loader-user_generic.so.0: ltt-facility-loader-user_generic.o
	@rm -f libltt-loader-user_generic.so libltt-loader-user_generic.so.0
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -lpthread -shared -Wl,-soname,libltt-loader-user_generic.so -o $@ $^
	ln -s libltt-loader-user_generic.so.0 libltt-loader-user_generic.so

%.o: %.c
	$(CC) $(LTT_CFLAGS) $(CFLAGS) -c -o $@ $+

.PHONY : clean install libs install_libs install_headers samples

install_headers:
	if [ ! -e "$(INCLUDE_DIR)/ltt" ] ; then mkdir $(INCLUDE_DIR)/ltt ; fi
	cp -f ltt/*.h $(INCLUDE_DIR)/ltt

install_libs:
	cp -df libltt-instrument-functions.so* libltt-instrument-functions.a $(LIB_DIR)
	cp -df libltt-usertrace-fast.so* libltt-usertrace-fast.a $(LIB_DIR)
	cp -df libltt-loader-user_generic.so* libltt-loader-user_generic.a $(LIB_DIR)

install: install_headers libs install_libs

clean:
	rm -fr *.o *~ sample-thread sample sample-highspeed sample-printf sample-instrument-fct libltt-instrument-functions.so* libltt-instrument-functions.a libltt-usertrace-fast.a libltt-usertrace-fast.so* libltt-loader-user_generic.so* libltt-loader-user_generic.a sample-thread-slow sample-thread-fast sample-thread-brand sample-block java/*.class java/Sample.h java/TestBrand.h
