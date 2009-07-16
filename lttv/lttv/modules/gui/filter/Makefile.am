#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(THREAD_LIBS)
AM_CFLAGS += -fvisibility=hidden
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_builddir}/lttv/modules/gui/lttvwindow/lttvwindow -llttvwindow

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguifilter.la
libguifilter_la_LDFLAGS = -module -avoid-version
libguifilter_la_SOURCES = filter.c

EXTRA_DIST = \
    hGuiFilterInsert.xpm
