#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

#libdir = ${lttlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

lib_LTLIBRARIES = liblttvtraceread.la
liblttvtraceread_la_SOURCES = tracefile.c markers.c
noinst_HEADERS = ltt-private.h
#event.c parser.c type.c
#parser.h

lttinclude_HEADERS = \
	compiler.h\
	markers.h\
	ltt.h\
	time.h\
	trace.h\
	ltt-types.h

EXTRA_DIST = crc32.tab
