#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
AM_CFLAGS += -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DPACKAGE_BIN_DIR=\""$(bindir)"\"
AM_CFLAGS += -fvisibility=hidden
LIBS += $(GLIB_LIBS)
LIBS += $(THREAD_LIBS)
LIBS += $(GTK_LIBS) -L${top_builddir}/lttv/modules/gui/lttvwindow/lttvwindow -llttvwindow
LIBS += $(UTIL_LIBS)

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguitracecontrol.la
libguitracecontrol_la_LDFLAGS = -module -avoid-version
libguitracecontrol_la_SOURCES = tracecontrol.c

EXTRA_DIST = \
    hTraceControlInsert.xpm TraceControlStart.xpm TraceControlPause.xpm \
		TraceControlStop.xpm
