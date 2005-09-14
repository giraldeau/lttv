#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
AM_CFLAGS += -DPACKAGE_DATA_DIR=\""$(datadir)"\"
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_srcdir}/lttv/modules/gui/lttvwindow/lttvwindow -llttvwindow

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguitracecontrol.la
libguitracecontrol_la_LDFLAGS = -module
libguitracecontrol_la_SOURCES = tracecontrol.c

EXTRA_DIST = \
    hTraceControlInsert.xpm
    

