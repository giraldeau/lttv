#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

#libdir = ${lttlibdir}

AM_CFLAGS = $(PACKAGE_CFLAGS)

if BUILD_JNI_INTERFACE
lib_LTLIBRARIES = liblttvtraceread.la liblttvtraceread_loader.la
liblttvtraceread_la_SOURCES = jni_interface.c tracefile.c marker.c event.c
liblttvtraceread_loader_la_SOURCES= lttvtraceread_loader.c
else
lib_LTLIBRARIES = liblttvtraceread.la
liblttvtraceread_la_SOURCES = tracefile.c marker.c event.c
endif

liblttvtraceread_la_LDFLAGS = -release 2.6
liblttvtraceread_loader_la_LDFLAGS = -release 2.6

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
