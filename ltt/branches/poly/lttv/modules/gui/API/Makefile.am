#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

libdir = ${lttlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS)

lib_LTLIBRARIES = libmainWinApi.la libcustomBox.la
libmainWinApi_la_SOURCES = toolbar.c menu.c gtkTraceSet.c
libcustomBox_la_SOURCES = gtkcustomhbox.c
EXTRA_DIST =
