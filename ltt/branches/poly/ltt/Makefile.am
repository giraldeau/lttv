#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

SUBDIRS = convert

libdir = ${lttlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

lib_LTLIBRARIES = libtraceread.la
libtraceread_la_SOURCES = event.c facility.c parser.c tracefile.c type.c
noinst_HEADERS = parser.h ltt-private.h


EXTRA_DIST = crc32.tab
