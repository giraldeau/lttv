CC=gcc
CFLAGS=-g -o
CMPFLAGS=-g -c
INCLUDEPATH=-Iinclude

#GLIB_CFLAGS = -I/usr/include/glib-1.2 -I/usr/lib/glib/include
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
#GLIB_CFLAGS = -I/usr/include/glib-2.0 

GLIB_CONFIG = /usr/bin/glib-config
GLIB_LIBS = -L/usr/lib -lglib

OBJ= convert.o


test: $(OBJ)
	$(CC) $(CFLAGS) test $(OBJ) $(GLIB_LIBS)

convert.o: convert.c 
	$(CC) $(INCLUDEPATH) $(GLIB_CFLAGS) $(CMPFLAGS) convert.c



clean:
	rm -rf *.o *~ *#


