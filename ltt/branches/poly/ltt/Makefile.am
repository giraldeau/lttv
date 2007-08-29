#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

#libdir = ${lttlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

lib_LTLIBRARIES = liblttvtraceread.la
liblttvtraceread_la_SOURCES = event.c parser.c tracefile.c type.c markers.c
noinst_HEADERS = parser.h ltt-private.h

lttinclude_HEADERS = \
  compiler.h\
	event.h\
	markers.h\
	ltt.h\
	time.h\
	trace.h\
	type.h\
	ltt-types.h

EXTRA_DIST = crc32.tab
