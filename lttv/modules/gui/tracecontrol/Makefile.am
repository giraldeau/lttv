#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

include ../common.am

AM_CFLAGS += -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DPACKAGE_BIN_DIR=\""$(bindir)"\"

lib_LTLIBRARIES = libguitracecontrol.la
libguitracecontrol_la_SOURCES = tracecontrol.c
libguitracecontrol_la_LIBADD = $(UTIL_LIBS)

EXTRA_DIST = \
    hTraceControlInsert.xpm TraceControlStart.xpm TraceControlPause.xpm \
		TraceControlStop.xpm
