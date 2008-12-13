#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

#libdir = ${lttlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)
LIBS += $(THREAD_LIBS)

lib_LTLIBRARIES = liblttvtraceread.la
liblttvtraceread_la_SOURCES = tracefile.c marker.c event.c
noinst_HEADERS = ltt-private.h

lttinclude_HEADERS = \
	compiler.h\
	marker.h\
	marker-field.h\
	ltt.h\
	time.h\
	trace.h\
	event.h\
	marker-desc.h\
	ltt-types.h
