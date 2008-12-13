#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
AM_CFLAGS += -fvisibility=hidden
LIBS += $(GLIB_LIBS)
LIBS += $(THREAD_LIBS)
LIBS += $(GTK_LIBS) -L${top_builddir}/lttv/modules/gui/lttvwindow/lttvwindow -llttvwindow

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguistatistics.la
libguistatistics_la_LDFLAGS = -module -avoid-version
libguistatistics_la_SOURCES = statistics.c

EXTRA_DIST = \
		hGuiStatisticInsert.xpm
