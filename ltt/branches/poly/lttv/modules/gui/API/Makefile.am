#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

libdir = ${lttvlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS)

lib_LTLIBRARIES = libcustomBox.la libmainWinApi.la
libcustomBox_la_SOURCES = gtkcustomhbox.c
libmainWinApi_la_SOURCES = toolbar.c menu.c gtkTraceSet.c
#libmainWinApi_la_LDFLAGS = -L${top_srcdir}/lttv/modules/gui/API -lcustomBox

EXTRA_DIST =
